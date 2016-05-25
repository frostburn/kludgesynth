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

static double param_a = 0;
static double param_b = 0;

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

#define NUM_BUTTONS (32)
static int index_by_button[NUM_BUTTONS];
static pthread_t joy_thread;

#define MAX_EVENTS (128)
static double joy_event_times[MAX_EVENTS];
static double joy_event_clocks[MAX_EVENTS];
static joy_clock_t last_joy_clock;
static int joy_event_index = -1;

static double joy_pitch_bend = 0;
static double joy_modulation = 0;
static double joy_velocity = 0.5;
static double joy_pitch_shift = 0;

static int program_mode = 0;
static int key_mod = 0;
static int octave = 0;
static int transpose = 0;

void handle_joy_event(const joy_event e)
{
    joy_clock_t joy_clock = e.clock;
    if (joy_event_index < 0) {
        last_joy_clock = joy_clock;
        joy_event_index = 0;
    }
    double event_t = next_t - last_t + t;
    event_t = get_event_time(
        event_t, next_t, 2 * next_t - last_t,
        joy_clock - last_joy_clock,
        MAX_EVENTS, &joy_event_index,
        joy_event_times, joy_event_clocks
    );
    #ifdef DEBUG
        print_joy_event(e);
        printf("%d: %g %g %g -> %g\n", e.clock, last_t, t, next_t, event_t);
    #endif
    if (e.type == JOY_BUTTON) {
        if (e.axis) {
            if (e.num == 10) {
                printf("Program mode on\n");
                program_mode = 1;
                return;
            }
            else if (program_mode) {
                program = e.num;
                printf("Selecting program %d\n", program);
                return;
            }
            int index = find_voice_index();
            index_by_button[e.num] = index;
            phases[index] = 0;
            freqs[index] = mtof(major_pentatonic(e.num, key_mod) + 60 + 12 * octave + transpose + joy_pitch_shift);
            velocities[index] = joy_velocity;
            on_times[index] = event_t;
            off_times[index] = 1e12;
            off_velocities[index] = 0;
        }
        else {
            if (e.num == 10) {
                printf("Program mode off\n");
                program_mode = 0;
                return;
            }
            int index = index_by_button[e.num];
            if (index >= 0) {
                off_times[index] = event_t;
                off_velocities[index] = 0.5;
            }
        }
    }
    else if (e.type == JOY_AXIS){
        if (!program_mode) {
            if (e.num == 0) {
                joy_pitch_bend = 1.0 * e.axis * JOY_AXIS_NORM;
            }
            else if (e.num == 1) {
                joy_modulation = 0.5 * e.axis * JOY_AXIS_NORM;
            }
            else if (e.num == 2) {
                param_a = 0.5 + 0.5 * e.axis * JOY_AXIS_NORM;
            }
            else if (e.num == 3 && 0) {  // Broken on my machine. :(
                joy_pitch_shift = e.axis * JOY_AXIS_NORM;
                printf("%g\n", joy_pitch_shift);
            }
            else if (e.num == 4) {
                joy_velocity = 0.5 - 0.5 * e.axis * JOY_AXIS_NORM;
            }
            else if (e.num == 5) {
                param_b = 0.5 + 0.5 * e.axis * JOY_AXIS_NORM;
            }
        }
        if (e.num == 6) {
            if (program_mode) {
                transpose += MAX(-1, MIN(1, e.axis));
                printf("Transpose %d\n", transpose);
            }
            else {
                key_mod = MAX(-1, MIN(1, e.axis));
            }
        }
        else if (e.num == 7) {
            if (program_mode) {
                octave -= MAX(-1, MIN(1, e.axis));
                printf("Octave %d\n", octave);
            }
        }
    }
}

void* joy_thread_function(void *args)
{
    while (1) {
        process_joy(handle_joy_event);
    }
}

void launch_joy()
{
    for (int i = 0; i < NUM_BUTTONS; i++) {
        index_by_button[i] = -1;
    }
    int status = pthread_create(&joy_thread, NULL, joy_thread_function, NULL);
    if (status == -1) {
        printf("Error: unable to create joy input thread.\n");
        exit(1);
    }
}

static double midi_event_times[MAX_EVENTS];
static double midi_event_clocks[MAX_EVENTS];
static clock_t last_midi_clock;
static int midi_event_index = -1;

#define NUM_PITCHES (128)
static int index_by_pitch[NUM_PITCHES];

static pthread_t midi_thread;

static double midi_pitch_bend = 0.0;
static double midi_modulation = 0.0;

void handle_midi_event(const midi_event e)
{
    clock_t midi_clock = clock();
    if (midi_event_index < 0) {
        last_midi_clock = midi_clock;
        midi_event_index = 0;
    }
    double event_t = next_t - last_t + t;
    event_t = get_event_time(
        event_t, next_t, 2 * next_t - last_t,
        midi_clock - last_midi_clock,
        MAX_EVENTS, &midi_event_index,
        midi_event_times, midi_event_clocks
    );
    #ifdef DEBUG
        print_midi_event(e);
        printf("%ld: %g, %g, %g -> %g\n", midi_clock, last_t, t, next_t, event_t);
    #endif
    if (e.type == 152) {
        int index = index_by_pitch[e.pitch];
        if (index >= 0) {
            off_times[index] = MIN(off_times[index], event_t);
            off_velocities[index] = e.velocity / 127.0;
        }
        index = find_voice_index();
        index_by_pitch[e.pitch] = index;
        phases[index] = 0;
        freqs[index] = mtof(e.pitch);
        velocities[index] = e.velocity / 127.0;
        on_times[index] = event_t;
        off_times[index] = 1e12;
        off_velocities[index] = 0;
    }
    else if (e.type == 136) {
        int index = index_by_pitch[e.pitch];
        if (index >= 0) {
            off_times[index] = event_t;
            off_velocities[index] = e.velocity / 127.0;
        }
    }
    else if (e.type == 184) {
        if (e.pitch == 1) {
            midi_modulation = e.velocity / 127.0;
        }
        else if (e.pitch == 120) {
            for (int i = 0; i < NUM_VOICES; i++) {
                off_times[i] = -1e12;
                off_velocities[i] = 0;
            }
        }
    }
    else if (e.type == 232) {
        midi_pitch_bend = (e.velocity - 64) / 32.0;
        if (e.pitch) {
            midi_pitch_bend = 2;
        }
    }
    else if (e.type == 200) {
        program = e.pitch;
        printf("Selecting program %d\n", program);
    }
}

void* midi_thread_function(void *args)
{
    while (1) {
        process_midi(handle_midi_event);
    }
}

void launch_midi()
{
    for (int i = 0; i < NUM_PITCHES; i++) {
        index_by_pitch[i] = -1;
    }
    int status = pthread_create(&midi_thread, NULL, midi_thread_function, NULL);
    if (status == -1) {
        printf("Error: unable to create midi input thread.\n");
        exit(1);
    }
}


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
        double pitch_bend = joy_pitch_bend + midi_pitch_bend;
        double modulation = joy_modulation + midi_modulation;
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
