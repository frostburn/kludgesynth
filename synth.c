#include <assert.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <math.h>
#include <portaudio.h>

// Compile with: gcc synth.c -pthread -lm -lportaudio -O3 -o synth
// Injection is best xD
#define MAIN
// #define DEV_NONBLOCK
#include "util.c"
#include "joydev.c"
#include "mididev.c"
#include "linreg.c"
#include "scale.c"
#include "clock_fit.c"

#define SAMPLE_RATE (44100)
#define SAMPDELTA (1.0 / (double) SAMPLE_RATE)
#define FRAMES_PER_BUFFER (128)

#include "waveguide.c"
#include "osc.c"

#define NUM_VOICES (32)
#define NUM_PROGRAMS (3)
#define MAX_DECAY (5)

typedef struct
{
    char message[20];
}
paUserData;

static PaStream *stream;

static double t = 0;
static double last_t = 0;
static double next_t = 0;

static double on_times[NUM_VOICES];
static double off_times[NUM_VOICES];

static osc_state oscs[NUM_VOICES];
static kp_state karps[NUM_VOICES];

static int program = 0;

void init_voices()
{
    for (int i = 0; i < NUM_VOICES; i++) {
        on_times[i] = -A_LOT;
        off_times[i] = -A_LOT;

        oscs[i].phase = A_LOT;
        oscs[i].freq = 0;
        oscs[i].velocity = 0;
        oscs[i].off_velocity = 0;

        karps[i].num_samples = 0;
        karps[i].samples = NULL;
    }
}

int find_voice_index()
{
    double off_min = INFINITY;
    int index = 0;
    for (int i = 0; i < NUM_VOICES; i++) {
        if (off_times[i] < off_min) {
            off_min = off_times[i];
            index = i;
        }
    }
    return index;
}

void handle_note_on(int index, double event_t, double freq, double velocity)
{
    on_times[index] = event_t;
    off_times[index] = A_LOT;

    if (program == 0 || program == 1) {
        oscs[index].phase = 0;
        oscs[index].freq = freq;
        oscs[index].velocity = velocity;
    }
    if (program == 2) {
        kp_destroy(karps + index);
        karps[index].b0 = 0.5;
        karps[index].b1 = 0.5 - 4 / freq;
        karps[index].x1 = 0;
        kp_init(karps + index, freq);
        for (int i = 0; i < karps[index].num_samples; i++) {
            karps[index].samples[i] = frand() * sqrt(velocity);
        }
        for (int i = 0; i < 20 * karps[index].num_samples * (1 - velocity); i++) {
            kp_step(karps + index, 1);
        }
    }
}

void handle_note_off(int index, double event_t, double velocity)
{
    if (event_t < off_times[index]) {
        off_times[index] = event_t;

        if (program == 0 || program == 1) {
            oscs[index].off_velocity = velocity;
        }
        if (program == 2) {
            karps[index].b0 = 0.45;
        }
    }
}

#define MAX_EVENTS (128)
#include "handle_joy.c"
#include "handle_midi.c"

static int paCallback(
    const void *inputBuffer, void *outputBuffer,
    unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void *userData)
{
    paUserData *data = (paUserData*) userData;
    float *out = (float*) outputBuffer;
    double out_v;

    (void) timeInfo; /* Prevent unused variable warnings. */
    (void) statusFlags;
    (void) inputBuffer;

    last_t = t;
    next_t = t + framesPerBuffer * SAMPDELTA;

    for (int i = 0; i < framesPerBuffer; i++) {
        double pitch_bend = joy_state.pitch_bend + midi_state.pitch_bend;
        double modulation = joy_state.modulation + midi_state.modulation;
        double param_a = joy_state.param_a;
        double param_b = joy_state.param_b;
        double rate = itor(pitch_bend + 0.7 * sin(2 * M_PI * 6 * t) * modulation);
        out_v = 0;
        for (int j = 0; j < NUM_VOICES; j++) {
            double t_on = t - on_times[j];
            double t_off = t - off_times[j];
            if (t_off > MAX_DECAY || t_on < 0) {
                continue;
            }
            double v = 0;
            if (program == 0) {
                v = bowed_marimba(oscs[j], t, t_on, t_off, param_a, param_b);
                osc_step(oscs + j, rate);
            }
            else if (program == 1) {
                v = soft_ping(oscs[j], t, t_on, t_off, param_a, param_b);
                osc_step(oscs + j, rate);
            }
            else if (program == 2) {
                v = kp_step(karps + j, rate);
            }
            out_v += v;
        }

        t += SAMPDELTA;

        *out++ = (float) out_v;    /* left */
        *out++ = (float) out_v;    /* right */
    }
    return paContinue;
}

static void StreamFinished(void* userData)
{
    paUserData *data = (paUserData *) userData;
    printf("Stream Completed: %s\n", data->message);
}

int main(void)
{
    PaStreamParameters outputParameters;
    PaError err;
    paUserData data;
    int i;

    printf("PortAudio synth. SR = %d, BufSize = %d\n", SAMPLE_RATE, FRAMES_PER_BUFFER);

    err = Pa_Initialize();
    if(err != paNoError) goto error;

    outputParameters.device = Pa_GetDefaultOutputDevice(); /* default output device */
    if (outputParameters.device == paNoDevice) {
        fprintf(stderr,"Error: No default output device.\n");
        goto error;
    }
    outputParameters.channelCount = 2;         /* stereo output */
    outputParameters.sampleFormat = paFloat32; /* 32 bit floating point output */
    outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;

    err = Pa_OpenStream(
        &stream,
        NULL, /* no input */
        &outputParameters,
        SAMPLE_RATE,
        FRAMES_PER_BUFFER,
        paNoFlag,
        paCallback,
        &data
    );
    if(err != paNoError) goto error;

    sprintf(data.message, "No Message");
    err = Pa_SetStreamFinishedCallback(stream, &StreamFinished);
    if(err != paNoError) goto error;

    init_voices();

    if (open_joy(NULL)) {
        joy_description joy_desc;
        init_joy(&joy_desc);
        print_joy_description(joy_desc);
        launch_joy();
    }
    else {
        printf("Joy disabled.\n");
    }

    if (init_midi(NULL)) {
        launch_midi();
    }
    else {
        printf("Midi disabled.\n");
    }

    err = Pa_StartStream(stream);
    if(err != paNoError) goto error;

    printf("Synth initialized!\n");
    printf("Press ENTER key to quit.\n");  
    getchar();

    err = Pa_StopStream(stream);
    if(err != paNoError) goto error;

    err = Pa_CloseStream(stream);
    if(err != paNoError) goto error;

    Pa_Terminate();
    printf("K thx bye!\n");

    return err;
    error:
    Pa_Terminate();
    fprintf(stderr, "An error occured while using the portaudio stream\n");
    fprintf(stderr, "Error number: %d\n", err);
    fprintf(stderr, "Error message: %s\n", Pa_GetErrorText(err));
    return err;
}
