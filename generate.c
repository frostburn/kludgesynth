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

    double phase = 0;

    double max_amplitude = 0;
    for (int i = 0; i < NUM_SAMPLES; i++) {
        double t = i * SAMPDELTA;

        double freq = 220.0 + sine(t * 2.5) * 10.0 + 100 * t;
        phase += SAMPDELTA * freq;
        double v = 0;
        v += formant(phase, (1000 + 200 * sine(t)) / freq, 2);
        v += formant(phase, (2000 - t * 100) / freq, 2);

        samples[i] = v * 0.4;

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
