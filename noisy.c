#define NUM_SUB_VOICES (4)

typedef struct noisy_state
{
    double freq;
    double velocity;
    double noisiness;
    double noise_rate;
    double phases[NUM_SUB_VOICES];
    snow_state snows[NUM_SUB_VOICES];
} noisy_state;

void noisy_init(noisy_state *n)
{
    for (int i = 0; i < NUM_SUB_VOICES; i++) {
        n->phases[i] = frand();
        snow_init(n->snows + i);
    }
}

double noisy_step(noisy_state *n, double rate, double waveform(double))
{
    double v = 0;
    for (int i = 0; i < NUM_SUB_VOICES; i++) {
        snow_step(n->snows + i, n->noise_rate);
        n->phases[i] += SAMPDELTA * n->freq * (rate + snow_linear(n->snows[i]) * n->noisiness);
        v += waveform(n->phases[i]);
    }
    return v * n->velocity * 0.1;
}
