#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#define MAIN
#include "util.c"
#include "waveform.c"
#include "interpolation.c"

#define SAMPLE_RATE (44100)
#define SAMPDELTA (1.0 / (double) SAMPLE_RATE)
#define NUM_SAMPLES (SAMPLE_RATE * 6)

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

    blit_state blit;
    blit.phase = 0;
    blit.freq = 220.0;

    filter_state filter;
    filter_reset(&filter);

    double max_amplitude = 0;
    for (int i = 0; i < NUM_SAMPLES; i++) {
        double t = i * SAMPDELTA;

        // filter_bpf(&filter, 220.0 + 200 * t, 50);
        // double v = filter_step(&filter, sineblit_step(&blit, 1));
        // samples[i] = v * 0.005;

        samples[i] = formant(t * 200.0 + 0.3 * sine(700.41 * t), t * 3, 0.5) * 0.5;

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
