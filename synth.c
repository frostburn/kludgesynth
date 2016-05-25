#include <stdio.h>
#include <math.h>
#include <portaudio.h>

#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

#include <pthread.h>

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

#define NUM_VOICES (32)
#define NUM_PROGRAMS (2)

typedef struct
{
    char message[20];
}
paUserData;

static PaStream *stream;

static double t = 0;
static double last_t = 0;
static double next_t = 0;

static int voice_index = 0;
static double phases[NUM_VOICES];
static double freqs[NUM_VOICES];
static double velocities[NUM_VOICES];
static double on_times[NUM_VOICES];
static double off_velocities[NUM_VOICES];
static double off_times[NUM_VOICES];

static int program = 0;

void init_voices() {
    for (int i = 0; i < NUM_VOICES; i++) {
        phases[i] = A_LOT;
        freqs[i] = 0;
        velocities[i] = 0;
        off_velocities[i] = 0;
        on_times[i] = -A_LOT;
        off_times[i] = -A_LOT;
    }
}

int find_voice_index() {
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

#define MAX_EVENTS (128)
#include "handle_joy.c"
#include "handle_midi.c"



static int patestCallback(
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
        double f_delta = itor(pitch_bend + sin(2 * M_PI * 7 * t) * modulation) * SAMPDELTA;
        out_v = 0;
        for (int j = 0; j < NUM_VOICES; j++) {
            double t_on = t - on_times[j];
            double t_off = t - off_times[j];
            if (t_off > 4 || t_on < 0) {
                continue;
            }
            double p = phases[j] * 2 * M_PI;
            double v = 0;
            if (program == 0) {
                v = 0.4 * velocities[j] * sin(
                    p +
                    sin(p * 5) * (0.1 + 0.3 * velocities[j]) +
                    0.1 * sin(p) +
                    0.8 * param_a * sin(p * 3) +
                    1.0 * param_b * sin(p * 2)
                ) * exp(-t_on * 0.02) * tanh(t_on * 200);
                if (t_off >= 0) {
                    v *= exp(-t_off * 10);
                }
            }
            else if (program == 1) {
                double a = 0.01 + 2 * param_a;
                double b = param_b;
                double t_eff;
                if (t_off < 0) {
                    t_eff = t_on * 4;
                }
                else {
                    t_eff = (t_on - t_off) * 4 + t_off * 2;
                }
                double d = exp(-t_eff);
                a *= d;
                b *= d;
                v = 0.4 * velocities[j] * tanh(sin(p + b * sin(p + 3 * t)) * a) / tanh(a) * d * tanh(t_on * (300 + 200 * velocities[j]));
            }
            out_v += v;

            phases[j] += freqs[j] * f_delta;
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
        patestCallback,
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
