#include <stdio.h>
#include <stdlib.h>

#define MAIN
#include "util.c"

#define SAMPLE_RATE (44100)
#define SAMPDELTA (1.0 / (double) SAMPLE_RATE)
#define NUM_SAMPLES (SAMPLE_RATE * 3)

#include "filter.c"

int main()
{
    double *samples = malloc(NUM_SAMPLES * sizeof(double));

    filter_state filter;

    double max_amplitude = 0;
    for (int i = 0; i < NUM_SAMPLES; i++) {
        filter_lpf(&filter, i * SAMPDELTA * 1000 + 100, 1000);
        samples[i] = filter_step(&filter, frand() * 0.1);

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
    fwrite(samples, sizeof(double),NUM_SAMPLES, f);
    fclose(f);
    return 0;
}