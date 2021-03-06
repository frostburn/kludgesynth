typedef struct kp_state {
    double b0;
    double b1;
    double x1;
    double rate;
    double mu;
    double z0;
    double z1;
    int num_samples;
    double *samples;
    int index;
} kp_state;

void kp_preinit(kp_state *kp)
{
    kp->num_samples = 0;
    kp->samples = NULL;
    kp->index = 0;
}

void kp_init(kp_state *kp, double freq)
{
    kp->mu = 0;
    kp->z0 = kp->x1;
    kp->z1 = kp->z0;

    kp->num_samples = (int) (SAMPLE_RATE / freq);
    double filter_delay = fabs(kp->b1);  // Not really, but good enough.
    double effective_inverse_freq = (kp->num_samples + filter_delay) * SAMPDELTA;
    kp->rate = freq * effective_inverse_freq;

    kp->samples = malloc(kp->num_samples * sizeof(double));
    assert(kp->samples);
    kp->index = 0;
}

double _kp_step(kp_state *kp)
{
    if (!kp->samples) {
        return 0;
    }
    double x0 = kp->samples[kp->index];
    kp->samples[kp->index++] = x0 * kp->b0 + kp->x1 * kp->b1;
    kp->x1 = x0;
    if (kp->index >= kp->num_samples) {
        kp->index = 0;
    }
    return x0;
}

void kp_destroy(kp_state *kp)
{
    double *samples = kp->samples;
    kp_preinit(kp);
    free(samples);
}

double kp_step(kp_state *kp, double rate)
{
    kp->mu += kp->rate * rate;
    while (kp->mu >= 1) {
        kp->z0 = kp->x1;
        kp->z1 = _kp_step(kp);
        kp->mu -= 1;
    }
    return kp->z0 + kp->mu * (kp->z1 - kp->z0);
}
