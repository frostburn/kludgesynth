typedef struct blit_state
{
    double phase;
    double freq;
} blit_state;

// Returns sum(z ** i for i in range(n)) with linear fading of the last power.
complex double power_series_mu(complex double z, double n)
{
    complex double z1 = 1 - z;
    if (cabs(z1) < EPSILON) {
        return n;
    }
    double floor_n = floor(n);
    complex double z_floor_n = cpow(z, floor_n);
    return (1 - z_floor_n) / z1 + (n - floor_n) * z_floor_n;
}


complex double blit_step(blit_state *b, double rate, double softness)
{
    double freq = b->freq * rate;

    complex double v = power_series_mu(cexp(2 * M_PI * I * b->phase - softness), 0.5 * SAMPLE_RATE / freq);
    b->phase += SAMPDELTA * freq;
    return v;
}
