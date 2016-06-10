#include <assert.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <complex.h>
#include <math.h>
#include <portaudio.h>

// Compile with: gcc synth.c -pthread -lm -lportaudio -O3 -o synth
// Injection is best xD
#define MAIN
// #define DEV_NONBLOCK
#include "util.c"
#include "joydev.c"
#include "mididev.c"
#include "mousedev.c"
#include "kbdev.c"
#include "linreg.c"
#include "scale.c"
#include "clock_fit.c"
#include "waveform.c"
#include "interpolation.c"

#define SAMPLE_RATE (44100)
#define SAMPDELTA (1.0 / (double) SAMPLE_RATE)
#define FRAMES_PER_BUFFER (128)

#define RECORD_NUM_SAMPLES (SAMPLE_RATE * 60 * 10)

#include "filter.c"
#include "waveguide.c"
#include "osc.c"
#include "noise.c"
#include "noisy.c"
#include "additive.c"
#include "blit.c"

#include "instrument.c"
#include "percussion.c"

#define NUM_VOICES (16)
#define NUM_PROGRAMS (12)
#define NUM_MONO_PROGRAMS (3)
#define MAX_DECAY (5)

typedef struct
{
    size_t record_num_samples;
    size_t record_index;
    double *record_samples;
    char message[20];
}
paUserData;

static PaStream *stream;

static double t = 0;
static double last_t = 0;
static double next_t = 0;

static double last_pitch_bend = 0;
static double last_modulation = 0;
static double last_param_a = 0;
static double last_param_b = 0;

static double on_times[NUM_VOICES];
static double off_times[NUM_VOICES];
static int programs[NUM_VOICES];

static osc_state oscs[NUM_VOICES];
static kp_state karps[NUM_VOICES];
static snower_state snows[NUM_VOICES];
static noisy_state noisys[NUM_VOICES];
static pingsum_state pingsums[NUM_VOICES];
static pad_state pads[NUM_VOICES];
static blsaw_state blsaws[NUM_VOICES];
static pipe_state pipes[NUM_VOICES];
static voice_state voices[NUM_VOICES];

static double mono_on_time = -A_LOT;
static double mono_off_time = -A_LOT;
static double mono_a_on_time = -A_LOT;
static double mono_a_off_time = -A_LOT;
static double mono_b_on_time = -A_LOT;
static double mono_b_off_time = -A_LOT;

static osc_state mono_osc;
static voice_state mono_voice;
double last_mono_freq = 0.0;
double last_mono_param_a = 0.0;
double last_mono_param_b = 0.0;

static int mono_program = 0;

void init_voices()
{
    // TODO: Use preinits.
    for (int i = 0; i < NUM_VOICES; i++) {
        on_times[i] = -A_LOT;
        off_times[i] = -A_LOT;
        programs[i] = -1;

        oscs[i].phase = A_LOT;
        oscs[i].freq = 0;
        oscs[i].velocity = 0;
        oscs[i].off_velocity = 0;

        karps[i].num_samples = 0;
        karps[i].samples = NULL;

        pingsums[i].num_voices = 0;
        pingsums[i].pings = NULL;

        pads[i].snows = NULL;
        pads[i].base_amplitudes = NULL;
        pads[i].amplitudes = NULL;
        pads[i].rates = NULL;
        pads[i].coefs = NULL;

        blsaw_init(blsaws + i, 20.0);
        blsaws[i].velocity = 0;

        pipes[i].buffer.num_samples = 0;
        pipes[i].buffer.samples = NULL;

        voice_init(voices + i);
    }
    mono_osc.phase = 0;
    mono_osc.freq = 1;
    mono_osc.velocity = 0.5;
    mono_osc.off_velocity = 0.5;

    voice_init(&mono_voice);
    mono_voice.blit.freq = 1;
    mono_voice.velocity = 0.5;
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
    if (off_min < t) {
        return index;
    }
    return -1;
}

void handle_note_on(int index, int program, double event_t, double freq, double velocity)
{
    if (index < 0) {
        return;
    }
    on_times[index] = event_t;
    off_times[index] = A_LOT;
    programs[index] = program;

    if (program == 0 || program == 1 || program == 8) {
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
    if (program == 3 || program == 4) {
        snows[index].amplitude = velocity * 0.5;
        snows[index].mul = 1.0;
        snower_init(snows + index, freq);
    }
    if (program == 5) {
        noisys[index].freq = freq;
        noisys[index].velocity = velocity;
        noisys[index].noisiness = 0.15 * pow(freq, 0.7);
        noisys[index].noise_rate = (5 + rtoi(freq)) * SAMPDELTA;
        noisy_init(noisys + index);
    }
    if (program == 6) {
        pingsum_destroy(pingsums + index);
        pingsum_init(pingsums + index, 100);
        for (int i = 0; i < pingsums[index].num_voices; i++) {
            double k = i + 1;
            double e = 1.5 + log(freq) * 0.1;
            sineping_init(pingsums[index].pings + i, frand() * 0.1, freq * (pow(k, 0.6) + (0.2 + 0.03 * frand()) * i), 0.3 * velocity / (pow(k, e) + 0.1 * frand()), 0.01);
        }
    }
    if (program == 7) {
        pad_destroy(pads + index);
        pad_init(pads + index, 50);
        pads[index].phase = 0;
        pads[index].freq = freq;
        for (int i = 0; i < pads[index].num_voices; i++) {
            pads[index].base_amplitudes[i] = 0.1 * velocity / (i * i * i + 1);
            pads[index].amplitudes[i] = (0.2 + 0.3 * (i % 2)) * velocity * pow(i + 1, -0.7);
            pads[index].rates[i] = SAMPDELTA * (1 + 0.2 * i);
            pads[index].snows[i].mu = uniform();
        }
    }
    if (program == 9) {
        blsaw_init(blsaws + index, freq);
        blsaws[index].velocity = velocity;
    }
    if (program == 10) {
        blsaw_init(blsaws + index, 2 * freq);
        blsaws[index].velocity = velocity;
    }
    if (program == 11) {
        pipe_destroy(pipes + index);
        pipe_init(pipes + index, freq);
        pipes[index].velocity = velocity;
    }
    if (program == 12) {
        voices[index].blit.freq = freq;
        voices[index].velocity = velocity;
    }
}

void handle_note_off(int index, double event_t, double velocity)
{
    if (index < 0) {
        return;
    }
    int program = programs[index];
    if (event_t < off_times[index]) {
        off_times[index] = event_t;

        if (program == 0 || program == 1 || program == 8) {
            oscs[index].off_velocity = velocity;
        }
        if (program == 2) {
            karps[index].b0 = 0.45;
        }
        if (program == 3 || program == 4) {
            snows[index].mul = pow(0.004, SAMPDELTA);
        }
    }
}

#include "synth_perc.c"

void handle_mouse_click(int num, double event_t);

void handle_mouse_release(int num, double event_t)
{
    if (num == 4) {
        mono_off_time = event_t;
    }
    else if (num == 1) {
        mono_a_off_time = event_t;
    }
    else if (num == 2) {
        mono_b_off_time = event_t;
    }
}

#define MAX_EVENTS (128)
#include "handle_joy.c"
#include "handle_midi.c"
#include "handle_mouse.c"
#include "handle_kb.c"

void handle_mouse_click(int num, double event_t)
{
    if (num == 6) {
        mouse_state.x = 0;
        mouse_state.y = 0;
        mouse_state.wheel = 0;
        printf("Mouse reset\n");
    }
    else if (num == 4) {
        mono_on_time = event_t;
        mono_off_time = A_LOT;
    }
    else if (num == 5) {
        mono_program = (mono_program + 1) % NUM_MONO_PROGRAMS;
        printf("Selecting mono program %d\n", mono_program);
    }
    else if (num == 1) {
        mono_a_on_time = event_t;
        mono_a_off_time = A_LOT;
    }
    else if (num == 2) {
        mono_b_on_time = event_t;
        mono_b_off_time = A_LOT;
    }
    else {
        printf("Clicked button %d at %g.\n", num, event_t);
    }
}

// TODO: Calculate envelopes here and make them continuous.
double process_mono(double t_on, double t_off, double rate, double param_a, double param_b)
{
    double a_t_on = t - mono_a_on_time;
    double a_t_off = t - mono_a_off_time;
    double v = 0;
    double velocity = 0.5 + 0.5 * ferf(-mouse_state.y * 0.03);
    double freq = mtof(mouse_state.x * 0.05 + 60) * rate;
    if (mono_program == 0) {
        mono_osc.velocity = velocity;
        v = fm_meow(mono_osc, t, t_on, t_off, param_a, param_b);
        osc_step(&mono_osc, freq);
    }
    else if (mono_program == 1) {
        mono_voice.velocity = velocity * 0.8;
        v = voice_step(&mono_voice, t, t_on, t_off, freq, param_a, param_b);
    }
    else if (mono_program == 2) {
        last_mono_freq = freq = 0.97 * last_mono_freq + 0.03 * freq;
        last_mono_param_a = param_a = 0.97 * last_mono_param_a + 0.03 * param_a;
        last_mono_param_b = param_b = 0.97 * last_mono_param_b + 0.03 * param_b;
        mono_osc.velocity = velocity;
        v = formant_voice(mono_osc, freq, t, t_on, t_off, param_a, param_b, a_t_on, a_t_off);
        osc_step(&mono_osc, freq);
    }
    return v;
}

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
    (void) data;

    last_t = t;
    next_t = t + framesPerBuffer * SAMPDELTA;

    // TODO: Fix a crash when iterating all programs.

    for (int i = 0; i < framesPerBuffer; i++) {
        // TODO: Support pitch bend per channel.
        double pitch_bend = joy_state.pitch_bend + midi_state.pitch_bend[0];
        double modulation = joy_state.modulation + midi_state.modulation[0] + mouse_state.wheel * 0.05;
        double param_a = joy_state.param_a;
        double param_b = joy_state.param_b;

        // if (!mono_active) {
        //     param_a += mouse_state.x * 0.01;
        //     param_b -= mouse_state.y * 0.01;
        // }

        last_pitch_bend = pitch_bend = 0.9 * last_pitch_bend + 0.1 * pitch_bend;
        last_modulation = modulation = 0.9 * last_modulation + 0.1 * modulation;
        last_param_a = param_a = 0.9 * last_param_a + 0.1 * param_a;
        last_param_b = param_b = 0.9 * last_param_b + 0.1 * param_b;

        double rate = itor(pitch_bend + 0.7 * sin(2 * M_PI * 6 * t) * modulation);
        out_v = 0;
        for (int j = 0; j < NUM_VOICES; j++) {
            int program = programs[j];
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
            else if (program == 3) {
                v = snower_step_0(snows + j, rate);
            }
            else if (program == 4) {
                v = snower_step_1(snows + j, rate);
            }
            else if (program == 5) {
                double wf(double phase) {
                    double v = lissajous13(phase, param_a, param_b * 0.25);
                    if (t_off >= 0) {
                        v *= exp(-t_off * 10);
                    }
                    return v;
                }
                if (t_off < 0.2) {
                    v = noisy_step(noisys + j, rate, wf);
                }
            }
            else if (program == 6) {
                v = pingsum_step(pingsums + j, rate) * tanh(t_on * 150);
            }
            else if (program == 7) {
                v = pad_step_linear(pads + j, rate, 0.8 + 0.15 * param_a) * tanh(t_on * 10) * (1 - 0.4 * param_a);
                if (t_off >= 0) {
                    v *= exp(-t_off * 4);
                }
            }
            else if (program == 8) {
                v = softsaw_bass(oscs[j], t, t_on, t_off, param_a, param_b);
                osc_step(oscs + j, rate);
            }
            else if (program == 9 || program == 10) {
                double shift = param_b + 1;
                if (program == 10) {
                    shift -= 0.5;
                }
                v = blsaw_step(blsaws + j, rate, t_on, t_off, param_a * 0.2, shift);
            }
            else if (program == 11) {
                v = pipe_step(pipes + j, rate, t_off);
            }
            else if (program == 12) {
                v = voice_step(voices + j, t, t_on, t_off, rate, param_a, param_b);
            }
            out_v += v;
        }
        double t_on = t - mono_on_time;
        double t_off = t - mono_off_time;
        if (t_off <= MAX_DECAY || t_on < 0) {
            out_v += process_mono(t_on, t_off, rate, param_a, param_b);
        }

        t += SAMPDELTA;

        if (data->record_index < data->record_num_samples) {
            data->record_samples[data->record_index++] = out_v;
        }

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
    paUserData perc_data;

    data.record_num_samples = RECORD_NUM_SAMPLES;
    data.record_index = 0;
    data.record_samples = calloc(data.record_num_samples, sizeof(double));

    perc_data = data;
    perc_data.record_samples = calloc(perc_data.record_num_samples, sizeof(double));

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

    err = Pa_OpenStream(
        &perc_stream,
        NULL, /* no input */
        &outputParameters,
        SAMPLE_RATE,
        FRAMES_PER_BUFFER,
        paNoFlag,
        paPercCallback,
        &perc_data
    );
    if(err != paNoError) goto error;

    sprintf(data.message, "No Message");
    err = Pa_SetStreamFinishedCallback(stream, &StreamFinished);
    if(err != paNoError) goto error;

    waveform_init();
    init_voices();
    init_percussion();

    if (open_joy(NULL)) {
        joy_description joy_desc;
        init_joy(&joy_desc);
        print_joy_description(joy_desc);
        launch_joy();
    }
    else {
        printf("Joy disabled.\n");
    }

    int midi_fd = init_midi(NULL);
    if (midi_fd >= 0) {
        launch_midi(midi_fd);
    }
    else {
        printf("Midi disabled.\n");
    }

    int seq_fd = init_midi("/dev/snd/midiC3D0");
    if (seq_fd >= 0) {
        launch_midi(seq_fd);
    }
    else {
        printf("Sequencer disabled.\n");
    }

    if (init_mouse(NULL)) {
        launch_mouse();
    }
    else {
        printf("Mouse disabled.\n");
    }

    if (init_kb(NULL)) {
        launch_kb();
    }
    else {
        printf("Keyboard disabled.\n");
    }

    err = Pa_StartStream(stream);
    if(err != paNoError) goto error;

    err = Pa_StartStream(perc_stream);
    if(err != paNoError) goto error;

    printf("Synth initialized!\n");
    printf("Press ENTER key to quit.\n");  
    getchar();

    err = Pa_StopStream(stream);
    if(err != paNoError) goto error;

    err = Pa_StopStream(perc_stream);
    if(err != paNoError) goto error;

    err = Pa_CloseStream(stream);
    if(err != paNoError) goto error;

    err = Pa_CloseStream(perc_stream);
    if(err != paNoError) goto error;

    Pa_Terminate();

    close_joy();
    close_midi(midi_fd);
    close_midi(seq_fd);
    close_mouse();
    close_kb();

    FILE *f;
    f = fopen("dumps/record.raw", "wb");
    fwrite(data.record_samples, sizeof(double), data.record_index, f);
    fclose(f);

    printf("Recorded %g seconds of audio.\n", data.record_index * SAMPDELTA);

    printf("K thx bye!\n");

    return err;
    error:
    Pa_Terminate();
    fprintf(stderr, "An error occured while using the portaudio stream\n");
    fprintf(stderr, "Error number: %d\n", err);
    fprintf(stderr, "Error message: %s\n", Pa_GetErrorText(err));
    return err;
}
