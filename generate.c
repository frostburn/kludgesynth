#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#define MAIN
#include "util.c"
#include "waveform.c"
#include "interpolation.c"

#define SAMPLE_RATE (44100)
#define SAMPDELTA (1.0 / (double) SAMPLE_RATE)
#define NUM_SAMPLES (SAMPLE_RATE * 1)

#include "noise.c"
#include "blit.c"
#include "filter.c"
#include "additive.c"
#include "percussion.c"

// gcc generate.c -lm && ./a.out && aplay dumps/temp.raw --format=FLOAT_LE --rate=44100
int main()
{
    srand(time(NULL));
    seed_lcg(time(NULL));
    waveform_init();
    float *samples = calloc(NUM_SAMPLES, sizeof(float));

    polezero_state polezero;
    polezero_reset(&polezero);
    polezero_integrator(&polezero, 1000, 0.95);

    double max_amplitude = 0;
    for (int i = 0; i < NUM_SAMPLES; i++) {
        double t = i * SAMPDELTA;

        double f = 500.0;
        double v;
        v = triangle(t * f + 0.5 * exp(-t * 4) * triangle(t * f * 5.71)) + 0.1;
        v = polezero_step(&polezero, frand()) * v + 0.01 * v;
        v *= exp(-15 * t) * 3;
        samples[i] = v;

        if (i < 20) {
            printf("%g\n", samples[i]);
        }
        if (fabs(samples[i]) > max_amplitude) {
            max_amplitude = fabs(samples[i]);
        }
    }
    printf("<= %g\n", max_amplitude);
    FILE *f;
    f = fopen("dumps/temp.raw", "wb");
    fwrite(samples, sizeof(float), NUM_SAMPLES, f);
    fclose(f);
    return 0;
}
