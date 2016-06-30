#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#define MAIN
#include "util.c"
#include "waveform.c"
#include "interpolation.c"
#include "wavetable.c"

#define SAMPLE_RATE (44100)
#define SAMPDELTA (1.0 / (double) SAMPLE_RATE)
#define NUM_SAMPLES (SAMPLE_RATE * 4)

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

    int num_values = 100;
    double *values = malloc(num_values * sizeof(double));
    for (int i = 0; i < num_values; i++) {
        values[i] = frand();
    }

    wavetable wt = wavetable_linear(values, num_values);

    double max_amplitude = 0;
    for (int i = 0; i < NUM_SAMPLES; i++) {
        double t = i * SAMPDELTA;

        if (i % (SAMPLE_RATE / 6) == 0) {
            for (int i = 0; i < num_values; i++) {
                values[i] = frand();
            }
            wavetable_destroy(&wt);
            wt = wavetable_linear(values, num_values);
            wavetable_dc_block(&wt);
            wavetable_integrate(&wt);
        }

        double v = wavetable_eval(wt, t * num_values * 110.0);

        samples[i] = v * 0.1;

        if (i < 20) {
            printf("%g\n", samples[i]);
        }
        if (fabs(samples[i]) > max_amplitude) {
            max_amplitude = fabs(samples[i]);
        }
    }
    free(values);
    wavetable_destroy(&wt);

    printf("<= %g\n", max_amplitude);
    FILE *f;
    f = fopen("dumps/temp.raw", "wb");
    fwrite(samples, sizeof(float), NUM_SAMPLES, f);
    fclose(f);
    printf("Done\n");
    return 0;
}
