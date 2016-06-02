#include <stdio.h>
#include <math.h>
#ifndef MAIN
    #define MAIN
    #include "util.c"
    #undef MAIN
#endif

#define TABLE_SIZE (4096)

static double SINE_TABLE[TABLE_SIZE + 1];
static double EXP_TABLE[TABLE_SIZE + 1];
static double ERF_TABLE[TABLE_SIZE + 1];
void waveform_init()
{
    for(int i = 0; i < TABLE_SIZE + 1; i++) {
        double x = i / ((double) TABLE_SIZE);
        SINE_TABLE[i] = sin(2 * M_PI * x);
        ERF_TABLE[i] = erf(4 * x);
        EXP_TABLE[i] = exp(-16 * x);
    }
}

double sine(double phase)
{
    phase -= floor(phase);
    phase *= TABLE_SIZE;
    int index = (int) phase;
    double mu = phase - index;
    phase = SINE_TABLE[index];
    return phase + mu * (SINE_TABLE[index + 1] - phase);
}

double cosine(double phase)
{
    return sine(phase + 0.25);
}

double _ferf(double x)
{
    x *= 0.25 * TABLE_SIZE;
    int index = (int) x;
    if (index >= TABLE_SIZE) {
        return 1;
    }
    double mu = x - index;
    x = ERF_TABLE[index];
    return x + mu * (ERF_TABLE[index + 1] - x);
}

double ferf(double x)
{
    if (x < 0) {
        return -_ferf(-x);
    }
    return _ferf(x);
}

double fexp(double x)
{
    if (x > 0) {
        return exp(x);
    }
    x *= -0.0625 * TABLE_SIZE;
    int index = (int) x;
    if (index >= TABLE_SIZE) {
        return 0;
    }
    double mu = x - index;
    x = EXP_TABLE[index];
    return x + mu * (EXP_TABLE[index + 1] - x);
}

double softsaw(double phase, double sharpness)
{
    sharpness = clip(sharpness, EPSILON, 1.0 - EPSILON);
    return atan(sharpness * sine(phase) / (1.0 + sharpness * cosine(phase))) / asin(sharpness);
}


double lissajous12(double phase, double sharpness, double bias)
{
    sharpness = clip(sharpness, -1, 1);
    bias = clip(bias, EPSILON - 1, 1 - EPSILON);
    return atan2((1 + sharpness) * sine(phase), (1 - sharpness) * cosine(2 * phase + 0.5 * bias)) * 4.0 / (3.0 * M_PI);
}


double lissajous13(double phase, double sharpness, double bias)
{
    sharpness = clip(sharpness, -1, 1);
    bias = clip(bias, -0.5, 0.5);
    double x = phase - floor(phase + 0.5);
    return atan2((1 + sharpness) * sine(1.5 * x), (1 - sharpness) * cosine(0.5 * x + bias / 3.0)) * 2.0 / M_PI + x + x;
}

double saw(double phase)
{
    return 2 * (phase - floor(phase + 0.5));
}

double par(double phase)
{
    phase -= floor(phase + 0.5);
    return 0.5 - 6 * phase * phase;
}

double cub(double phase)
{
    phase -= floor(phase + 0.5);
    return phase * (5.19615242270663188 - 20.7846096908265275 * phase * phase);
}

double qua(double phase)
{
    phase -= floor(phase + 0.5);
    phase *= phase;
    return 0.875 - phase * (15 - 30 * phase);
}

double qui(double phase)
{
    phase -= floor(phase + 0.5);
    double x2 = phase * phase;
    return phase * (5.96255602510703402 - x2 * (34.0717487148973373 - 40.8860984578768047 * x2));
}

#ifndef MAIN
int main()
{
    waveform_init();
    double x = 5.3;
    for (size_t i = 0; i < 100000000; i++) {
        x = cos(-x);
    }
    printf("%g\n", x);
}
#endif
