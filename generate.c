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

// gcc generate.c -lm;./a.out;aplay dumps/temp.raw --format=FLOAT_LE --rate=44100
int main()
{
    waveform_init();
    float *samples = calloc(NUM_SAMPLES, sizeof(float));

    srand(time(NULL));
    #define NUM_VOICES (10)
    pingsum_state pingsum;
    pingsum_init(&pingsum, NUM_VOICES);
    double decay = 1e-16;
    sineping_init(pingsum.pings + 0, 0, 156, 0.05, decay * 0.1);
    sineping_init(pingsum.pings + 1, 0, 185, 0.25, decay);
    sineping_init(pingsum.pings + 2, 0, 467, 0.2, decay);
    sineping_init(pingsum.pings + 3, 0, 650, 0.07, decay);
    sineping_init(pingsum.pings + 4, 0, 773, 0.07, decay * 0.5);
    sineping_init(pingsum.pings + 5, 0, 961, 0.05, decay);
    sineping_init(pingsum.pings + 6, 0, 1111, 0.05, decay);
    sineping_init(pingsum.pings + 7, 0, 1211, 0.05, decay);
    sineping_init(pingsum.pings + 8, 0, 1311, 0.03, decay * 0.2);
    sineping_init(pingsum.pings + 9, 0, 1553, 0.03, decay * 0.2);

    pink_state pink;
    pink_init(&pink, 6);

    polezero_state polezero;
    polezero_reset(&polezero);
    polezero_apf(&polezero, -0.7);
    polezero.b1 -= 0.1;

    buffer_state buffer;
    buffer_init(&buffer, SAMPLE_RATE / 200);

    snare_state snare;
    snare_preinit(&snare);
    snare_step(&snare, 1);
    snare_destroy(&snare);
    snare_init(&snare);

    double max_amplitude = 0;
    for (int i = 0; i < NUM_SAMPLES; i++) {
        double t = i * SAMPDELTA;

        // samples[i] = pingsum_step(&pingsum, 1);
        // double v = pink_step(&pink) * 0.6 * exp(-t * 16);
        // v = polezero_step(&polezero, v + 0.4 * buffer_delay(buffer, buffer.num_samples));
        // buffer_step(&buffer, v);
        // samples[i] += v;

        //samples[i] = pink_step(&pink)

        samples[i] = snare_step(&snare, t);


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
