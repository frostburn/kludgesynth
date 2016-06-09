#include <stdio.h>
#include <math.h>
#ifndef MAIN
    #define MAIN
    #include "util.c"
    #undef MAIN
#endif

#define TABLE_SIZE (4096)
#define TABLE_SIZE_4 (1024)
#define TABLE_SIZE_16 (256)

static double SINE_TABLE[2 * TABLE_SIZE];
static double ERF_TABLE[2 * TABLE_SIZE];
static double EXP_TABLE[2 * TABLE_SIZE];
void waveform_init()
{
    for(int i = 0; i < TABLE_SIZE; i++) {
        // Solve for table constants:
        // target((i+0) / size) = table_0[i] + (i+0) * table_1[i]
        // target((i+1) / size) = table_0[i] + (i+1) * table_1[i]
        double x0 = i / ((double) TABLE_SIZE);
        double x1 = (i + 1) / ((double) TABLE_SIZE);
        double target_0 = sin(2 * M_PI * x0);
        double target_1 = sin(2 * M_PI * x1);
        // Use a single table for better cache behavior.
        SINE_TABLE[2*i + 0] = target_0 + i * (target_0 - target_1);
        SINE_TABLE[2*i + 1] = target_1 - target_0;

        target_0 = erf(4 * x0);
        target_1 = erf(4 * x1);
        ERF_TABLE[2*i + 0] = target_0 + i * (target_0 - target_1);
        ERF_TABLE[2*i + 1] = target_1 - target_0;

        target_0 = exp(-16 * x0);
        target_1 = exp(-16 * x1);
        EXP_TABLE[2*i + 0] = target_0 + i * (target_0 - target_1);
        EXP_TABLE[2*i + 1] = target_1 - target_0;
    }
}

double sine(double phase)
{
    phase -= floor(phase);
    phase *= TABLE_SIZE;
    int index = (int) phase;
    index += index;
    return SINE_TABLE[index] + phase * SINE_TABLE[index + 1];
}

double cosine(double phase)
{
    return sine(phase + 0.25);
}

double _ferf(double x)
{
    x *= TABLE_SIZE_4;
    int index = (int) x;
    if (index >= TABLE_SIZE) {
        return 1;
    }
    index += index;
    return ERF_TABLE[index] + x * ERF_TABLE[index + 1];
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
    x *= -TABLE_SIZE_16;
    int index = (int) x;
    if (index >= TABLE_SIZE) {
        return 0;
    }
    index += index;
    return EXP_TABLE[index] + x * EXP_TABLE[index + 1];
}

double softsaw(double phase, double sharpness)
{
    sharpness = clip(sharpness, EPSILON, 1.0 - EPSILON);
    return atan(sharpness * sine(phase) / (1.0 + sharpness * cosine(phase))) / asin(sharpness);
}

double softarc(double phase, double sharpness)
{
    if (sharpness < EPSILON) {
        return cosine(phase);
    }
    else if (sharpness < 1) {
        phase *= 0.5;
        return (hypot((1 + sharpness) * cosine(phase), (1 - sharpness) * sine(phase)) - 1) / sharpness;
    }
    else {
        return fabs(cosine(0.5 * phase)) * 2 - 1;
    }
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

double square(double phase, double bias)
{
    return 2 * (floor(phase) - floor(phase - 0.5 + bias)) - 1;
}

// Integral of floor(t)
// t * floor(t) - 0.5 * (floor(t) * floor(t + 1))

double triangle(double phase)
{
    return 4 * fabs(phase - floor(phase) - 0.5) - 1;
}

double parangle(double phase, double b)
{
    double x = phase - 0.5 * floor(2 * phase) - 0.25;
    x *= 16 * x;
    if (phase - floor(phase) < 0.5) {
        return x - 1;
    }
    return 1 - x;
}

double tooth(double phase)
{
    phase = tan(M_PI * phase);
    return tanh(phase * phase) * 2 -1;
}

double tri(double phase)
{
    phase -= floor(phase + 0.5);
    return tanh(tan(2 * M_PI * fabs(phase) - 0.5 * M_PI));
}

double trih(double phase)
{
    return erf(atanh(4 * fabs(phase - floor(phase + 0.5)) - 1.0));
}

double cosineh(double phase, double sharpness)
{
    if (sharpness < EPSILON) {
        return 0.5 + 0.5 * cosine(phase);
    }
    else if (sharpness < 0.99) {
        double a = sharpness / (1 - sharpness);
        return (cosh(a * cosine(0.5 * phase)) - 1) / (cosh(a) - 1);
    }
    else if (sharpness < 1) {
        phase -= floor(phase + 0.5);
        double a = -0.5 * M_PI * M_PI * sharpness / (1 - sharpness);
        return fexp(a * phase * phase);
    }
    else {
        return 0.0;
    }
}

double _formant(double phase, int ratio, double width)
{
    if (width < 700) {
        return cosh(cosine(0.5 * phase) * width) / cosh(width) * cosine(phase * ratio);
    }
    else {
        phase -= floor(phase + 0.5);
        return fexp(-0.5 * M_PI * M_PI * width * phase * phase) * cosine(phase * ratio);
    }
}

double formant(double phase, double ratio, double width)
{
    double floor_ratio = floor(ratio);
    double am = cosine(phase * floor_ratio);
    am += (ratio - floor_ratio) * (cosine(phase * (floor_ratio + 1)) - am);
    if (width < 700) {
        return cosh(cosine(0.5 * phase) * width) / cosh(width) * am;
    }
    else {
        phase -= floor(phase + 0.5);
        return fexp(-0.5 * M_PI * M_PI * width * phase * phase) * am;
    }
}

#ifndef MAIN
int main()
{
    waveform_init();

    double sine_error = 0;
    double erf_error = 0;
    double exp_error = 0;
    double e;
    for (double x = 0; x < 1; x += 1e-7) {
        e = fabs(sin(2 * M_PI * x) - sine(x));
        if (e > sine_error) {
            sine_error = e;
        }
        e = fabs(erf(10 * x - 5) - ferf(10 * x - 5));
        if (e > erf_error) {
            erf_error = e;
        }
        e = fabs(exp(-20 * x) - fexp(-20 * x));
        if (e > exp_error) {
            exp_error = e;
        }
    }
    printf("Sine error %g\n", sine_error);
    printf("Erf error %g\n", erf_error);
    printf("Exp error %g\n", exp_error);
    return 0;

    double x = 5.3;
    for (size_t i = 0; i < 1000000000; i++) {
        x = sine(x);
    }
    printf("%g\n", x);
}
#endif
