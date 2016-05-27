typedef struct convolver_state {
    int num_samples;
    int index;
    double *samples;
    double *coefs;
} convolver_state;

void convolver_init(convolver_state *c, int num_samples)
{
    c->num_samples = num_samples;
    c->index = 0;
    c->samples = malloc(num_samples * sizeof(double));
    c->coefs = malloc(num_samples * sizeof(double));
}

void convolver_destroy(convolver_state *c)
{
    free(c->samples);
    free(c->coefs);
}

double convolver_step(convolver_state *c, double sample)
{
    double v = 0;
    c->samples[c->index] = sample;
    for (int i = 0; i < c->num_samples; i++) {
        v += c->coefs[i] * c->samples[(i + c->index) % c->num_samples];
    }
    c->index++;
    if (c->index >= c->num_samples) {
        c->index = 0;
    }
    return v;
}
