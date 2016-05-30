#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#define EPSILON (1e-10)
#define A_LOT (1e12)
#define TWO_ROOT_TWELVE (1.0594630943592953)
#define I_LOG_TWO_ROOT_TWELVE (17.312340490667548)
#define PITCH_FIX (36.37631656229583)

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#define CLIP(x, min, max) ((x) < (min)) ? (min) : (((x) > (max)) ? (max) : (x))

double clip(double x, double min, double max)
{
    if (x < min) {
        return min;
    }
    if (x > max) {
        return max;
    }
    return x;
}

// Midi pitch to frequency
double mtof(double pitch)
{
    // Same as 440.0 * pow(2.0, (pitch - 69) / 12.0);
    return pow(TWO_ROOT_TWELVE, pitch + PITCH_FIX);
}

// Frequency to midi pitch
double ftom(double freq)
{
    return log(freq) * I_LOG_TWO_ROOT_TWELVE - PITCH_FIX;
}

// Midi interval to frequency ratio
double itor(double interval)
{
    return pow(TWO_ROOT_TWELVE, interval);
}

// Frequency ratio to midi interval
double rtoi(double ratio)
{
    return log(ratio) * I_LOG_TWO_ROOT_TWELVE;
}

double tritave_itor(double interval)
{
    return pow(3.0, interval / 19.0);
}

double bohlen_pierce_itor(double interval)
{
    return pow(3.0, interval / 13.0);
}

size_t ceil_div(size_t x, size_t y)
{
    if (x == 0) {
        return 0;
    }
    return 1 + ((x - 1) / y);
}

double frand()
{
    return 2.0 * (rand() / (double) RAND_MAX) - 1.0;
}

double uniform()
{
    return rand() / (double) RAND_MAX;
}

#ifndef MAIN
int main()
{
    for (double i = 0; i < 10; i += 0.5 + frand()) {
        printf("%g -> %g -> %g\n", i, itor(i), rtoi(itor(i)));
        printf("%g -> %g -> %g\n", i + 60, mtof(i + 60), mtof(ftom(i + 60)));
    }
    return 0;
}
#endif
