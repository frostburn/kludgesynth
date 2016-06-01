typedef struct pad_state
{
    double phase;
    double freq;
    int num_voices;
    snow_state *snows;
    double *base_amplitudes;
    double *amplitudes;
    double *rates;
    double *coefs;
} pad_state;

typedef struct blsaw_state
{
    double velocity;
    double off_velocity;
    double shift_phase;
    blit_state blit;
    polezero_state polezero;
} blsaw_state;

typedef struct voice_state
{
    double velocity;
    double off_velocity;
    blit_state blit;
    filter_state filters[3];
} voice_state;

typedef struct pipe_state
{
    double velocity;
    double rate;
    double mu;
    buffer_state buffer;
} pipe_state;

void pad_init(pad_state *p, int num_voices)
{
    p->num_voices = num_voices;
    p->snows = malloc(num_voices * sizeof(snow_state));
    assert(p->snows);
    p->base_amplitudes = malloc(num_voices * sizeof(double));
    assert(p->base_amplitudes);
    p->amplitudes = malloc(num_voices * sizeof(double));
    assert(p->amplitudes);
    p->rates = malloc(num_voices * sizeof(double));
    assert(p->rates);
    p->coefs = malloc(num_voices * sizeof(double));
    assert(p->coefs);

    for (int i = 0; i < num_voices; i++) {
        snow_init(p->snows + i);
    }
}

void pad_destroy(pad_state *p) {
    free(p->snows);
    free(p->amplitudes);
    free(p->base_amplitudes);
    free(p->rates);
    free(p->coefs);
}

double pad_step_linear(pad_state *p, double rate, double sharpness)
{
    sharpness = tanh(sharpness);
    if (!p->num_voices) {
        return 0;
    }
    for (int i = 0; i < p->num_voices; i++) {
        snow_step(p->snows + i, p->rates[i]);
        p->coefs[i] = snow_linear(p->snows[i]) * p->amplitudes[i] + p->base_amplitudes[i];
    }
    p->phase += SAMPDELTA * p->freq * rate;
    return cis_sum_real(p->phase, sharpness, p->num_voices, p->coefs);
}

void blsaw_init(blsaw_state *blsaw, double freq)
{
    blsaw->shift_phase = 0;
    blsaw->blit.phase = 0;
    blsaw->blit.freq = freq;
    polezero_reset(&blsaw->polezero);
    polezero_integrator(&blsaw->polezero, freq, 0.95);
}

double blsaw_step(blsaw_state *blsaw, double rate, double t_on, double t_off, double softness, double shift)
{
    if (t_off > 0.1) {
        return 0;
    }
    if (t_off < 0) {
        t_off = 0;
    }
    if (softness < 0) {
        softness = 0;
    }
    complex double z = cexp(2 * M_PI * I * blsaw->shift_phase + 1.5 * softness - 500 * t_off) * tanh(500 * t_on);
    double v = polezero_step(&blsaw->polezero, cimag(blit_step(&blsaw->blit, rate, softness) * z) * blsaw->velocity);
    blsaw->shift_phase += blsaw->blit.freq * rate * shift * SAMPDELTA;
    return v;
}

void voice_init(voice_state *voice)
{
    voice->blit.phase = 0;
    voice->blit.freq = 0;
    for (int i = 0; i < 3; i++) {
        filter_reset(voice->filters + i);
    }
}

double voice_step(voice_state *voice, double t, double t_on, double t_off, double rate, double formant_a, double formant_b)
{
    if (t_off > 0.1) {
        return 0;
    }
    if (t_off < 0) {
        t_off = 0;
    }
    double normalizer = voice->blit.freq * rate * SAMPDELTA;
    filter_lpf(voice->filters + 0, 400, 10000);
    filter_bpf(voice->filters + 1, 400 + 400 * formant_a, 300);
    filter_bpf(voice->filters + 2, 1100 + 900 * formant_b, 400);
    double v = sineblit_step(&voice->blit, rate) * normalizer;
    v = filter_step(voice->filters + 0, v);
    v = filter_step(voice->filters + 1, v) * (2.5 - 0.9 * formant_a) + filter_step(voice->filters + 2, v) * (2.2 - 1.5 * formant_b);
    return tanh(v * 2.5 * voice->velocity * tanh(50 * t_on) * exp(-50 * t_off));
}

void pipe_init(pipe_state *p, double freq)
{
    freq *= 2;
    int num_samples = (int) (SAMPLE_RATE / freq);
    buffer_init(&p->buffer, num_samples);
    double effective_inverse_freq = (num_samples - 0.2) * SAMPDELTA;
    p->rate = freq * effective_inverse_freq;
    p->mu = 0;
}

void pipe_destroy(pipe_state *p)
{
    buffer_destroy(&p->buffer);
}

double pipe_step(pipe_state *p, double rate, double t_off)
{
    if (!p->buffer.samples) {
        return 0;
    }
    p->mu += p->rate * rate;
    while (p->mu >= 1) {
        double exitation = 0;
        if (t_off < 0) {
            exitation = frand() * 0.03 * p->velocity;
        }
        double y = exitation - 0.499 * (buffer_delay(p->buffer, p->buffer.num_samples) + buffer_delay(p->buffer, p->buffer.num_samples - 1));
        buffer_step(&p->buffer, y);
        p->mu -= 1;
    }
    double z0 = buffer_delay(p->buffer, 1);
    double z1 = buffer_delay(p->buffer, 2);
    return z0 + p->mu * (z1 - z0);
}
