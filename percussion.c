#define SNARE_NUM_VOICES (10)

typedef struct snare_state
{
    double velocity;
    pingsum_state pingsum;
    pink_state pink;
    polezero_state polezero;
    buffer_state buffer;
} snare_state;

typedef struct hihat_state
{
    double velocity;
    polezero_state polezero;
} hihat_state;

void snare_preinit(snare_state *s)
{
    pingsum_preinit(&s->pingsum);
    pink_preinit(&s->pink);
    buffer_preinit(&s->buffer);
}

void snare_init(snare_state *s, double velocity)
{
    s->velocity = velocity;
    pingsum_init(&s->pingsum, SNARE_NUM_VOICES);
    double decay = 1e-16;
    sineping_init(s->pingsum.pings + 0, 0, 156, 0.05, decay * 0.1);
    sineping_init(s->pingsum.pings + 1, 0, 185, 0.25, decay);
    sineping_init(s->pingsum.pings + 2, 0, 467, 0.2, decay);
    sineping_init(s->pingsum.pings + 3, 0, 650, 0.07, decay);
    sineping_init(s->pingsum.pings + 4, 0, 773, 0.07, decay * 0.5);
    sineping_init(s->pingsum.pings + 5, 0, 961, 0.05, decay);
    sineping_init(s->pingsum.pings + 6, 0, 1111, 0.05, decay);
    sineping_init(s->pingsum.pings + 7, 0, 1211, 0.05, decay);
    sineping_init(s->pingsum.pings + 8, 0, 1311, 0.03, decay * 0.2);
    sineping_init(s->pingsum.pings + 9, 0, 1553, 0.03, decay * 0.2);

    pink_init(&s->pink, 6);

    polezero_reset(&s->polezero);
    polezero_apf(&s->polezero, -0.7);
    s->polezero.b1 -= 0.1;

    buffer_init(&s->buffer, SAMPLE_RATE / 200);
}

void snare_destroy(snare_state *s)
{
    pingsum_destroy(&s->pingsum);
    pink_destroy(&s->pink);
    buffer_destroy(&s->buffer);
}

double snare_step(snare_state *s, double on_t)
{
    if (on_t > 1) {
        return 0;
    }
    double drum = pingsum_step(&s->pingsum, 1);
    double noise = pink_step(&s->pink) * 0.6 * fexp(-on_t * 16);
    noise = polezero_step(&s->polezero, noise + 0.4 * buffer_delay(s->buffer, s->buffer.num_samples));
    buffer_step(&s->buffer, noise);
    return (drum + noise) * s->velocity;
}

void hihat_init(hihat_state *h, double velocity)
{
    h->velocity = velocity;
    polezero_reset(&h->polezero);
    polezero_integrator(&h->polezero, 1000, 0.87);
}

double hihat_step(hihat_state *h, double on_t)
{
        double t = on_t;
        double f = 520.0;
        double v;
        v = triangle(t * f + (0.4 + 0.1 * h->velocity) * exp(-t * 5) * triangle(t * f * 6.61)) + 0.2;
        v = polezero_step(&h->polezero, frand()) * v + 0.01 * v;
        v *= exp(-15 * t) * 5 * h->velocity;
        return v;
}
