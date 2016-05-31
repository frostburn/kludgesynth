#include <complex.h>

typedef struct blit_state
{
    double phase;
    double freq;
} blit_state;

double sine_series_mu(double phase, double n)
{
    n += 1;
    double floor_n = floor(n);
    double mu = n - floor_n;
    floor_n *= phase;
    double s = sine(0.5 * floor_n);
    return -tan(M_PI * (phase + 0.5)) * s * s + sine(floor_n) * (mu - 0.5);
}

double sine_series_odd_mu(double phase, double n)
{
    double s = sine(phase);
    if (fabs(s) < EPSILON) {
        return s * n * n;
    }
    double floor_n = floor(n);
    double mu = n - floor_n;
    floor_n *= 2 * phase;
    return (0.5 - 0.5 * cosine(floor_n)) / s + sine(floor_n + phase) * mu;
}

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
