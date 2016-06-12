#define NUM_MONO_PROGRAMS (3)

static PaStream *mono_stream;

static double mono_on_time = -A_LOT;
static double mono_off_time = -A_LOT;
static double mono_a_on_time = -A_LOT;
static double mono_a_off_time = -A_LOT;
static double mono_b_on_time = -A_LOT;
static double mono_b_off_time = -A_LOT;

static envelope_state mono_env;
static envelope_state mono_mod_a;
static envelope_state mono_mod_b;

static osc_state mono_osc;
static voice_state mono_voice;
double mono_velocity = 0.0;
double last_mono_pitch = 0.0;
double last_mono_velocity = 0.0;
double last_mono_param_a = 0.0;
double last_mono_param_b = 0.0;

static int mono_program = 0;

void init_mono()
{
    envelope_init(&mono_env, 0.03, 0, 1, 0.01, 0.2);
    envelope_init(&mono_mod_a, 0.04, 0, 1, 0.02, 0.2);
    envelope_init(&mono_mod_b, 0.04, 0, 1, 0.04, 0.2);

    mono_osc.phase = 0;
    mono_osc.freq = 1;
    mono_osc.velocity = 0.5;
    mono_osc.off_velocity = 0.5;

    voice_init(&mono_voice);
    mono_voice.blit.freq = 1;
    mono_voice.velocity = 0.5;
}

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

void handle_mouse_delta_x(double delta)
{
    return;
}

void handle_mouse_delta_y(double delta)
{
    mono_velocity -= 0.03 * delta;
    if (mono_velocity < 0) {
        mono_velocity = 0;
    }
    mono_velocity = ferf(mono_velocity);
}

double process_mono(double rate, double param_a, double param_b)
{
    double env = erf_env_step(&mono_env, mono_t, mono_on_time, mono_off_time);
    double mod_a = erf_env_step(&mono_mod_a, mono_t, mono_a_on_time, mono_a_off_time);
    double mod_b = erf_env_step(&mono_mod_b, mono_t, mono_b_on_time, mono_b_off_time);
    double v = 0;
    double velocity = mono_velocity;
    last_mono_velocity = velocity = 0.96 * last_mono_velocity + 0.04 * velocity;
    double mono_pitch = mouse_state.x * 0.05 + 60;
    last_mono_pitch = mono_pitch = 0.94 * last_mono_pitch + 0.06 * mono_pitch;
    double freq = mtof(mono_pitch) * rate;
    if (mono_program == 0) {
        mono_osc.velocity = velocity;
        v = fm_meow(mono_osc, param_a, param_b, mod_a, mod_b) * env;
        osc_step(&mono_osc, freq);
    }
    else if (mono_program == 1) {
        mono_voice.velocity = velocity * 0.8;
        v = voice_step(&mono_voice, freq, env, param_a, param_b, mod_a, mod_b);
    }
    else if (mono_program == 2) {
        mono_osc.velocity = velocity;
        v = formant_voice(mono_osc, mono_t, freq, param_a, param_b, mod_a, mod_b) * env;
        osc_step(&mono_osc, freq);
    }
    return v;
}

static int paMonoCallback(
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

    last_mono_t = mono_t;
    next_mono_t = mono_t + framesPerBuffer * SAMPDELTA;


    for (int i = 0; i < framesPerBuffer; i++) {
        out_v = 0;
        double pitch_bend = joy_state.pitch_bend + midi_state.pitch_bend[0];
        double modulation = joy_state.modulation + midi_state.modulation[0] + mouse_state.wheel * 0.05;
        double param_a = joy_state.param_a;
        double param_b = joy_state.param_b;

        last_mono_param_a = param_a = 0.96 * last_mono_param_a + 0.04 * param_a;
        last_mono_param_b = param_b = 0.96 * last_mono_param_b + 0.04 * param_b;

        double rate = itor(pitch_bend + 0.7 * sin(2 * M_PI * 6 * mono_t) * modulation);

        double t_on = mono_t - mono_on_time;
        double t_off = mono_t - mono_off_time;
        if (t_off <= MAX_DECAY || t_on < 0) {
            out_v = process_mono(rate, param_a, param_b);
        }

        mono_t += SAMPDELTA;

        if (data->record_index < data->record_num_samples) {
            data->record_samples[data->record_index++] = out_v;
        }

        *out++ = (float) out_v;    /* left */
        *out++ = (float) out_v;    /* right */
    }
    return paContinue;
}
