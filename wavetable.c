typedef struct wavetable
{
    size_t num_coefs;
    size_t degree;
    double *coefs;
} wavetable;

wavetable wavetable_new(size_t num_coefs, size_t degree)
{
    wavetable wt = {num_coefs, degree, NULL};
    wt.coefs = malloc(num_coefs * (degree + 1) * sizeof(double));
    return wt;
}

wavetable wavetable_linear(double *values, size_t num_values)
{
    wavetable wt = wavetable_new(num_values, 1);
    for (size_t i = 0; i < num_values; i++) {
        wt.coefs[2*i + 0] = values[i];
        wt.coefs[2*i + 1] = values[(i + 1) % num_values] - values[i];
    }
    return wt;
}

void wavetable_dc_block(wavetable *wt)
{
    double integral = 0;
    for (size_t i = 0; i < wt->num_coefs; i++) {
        size_t index = i * (wt->degree + 1);
        for (size_t j = 0; j <= wt->degree; j++) {
            integral += wt->coefs[index + j] / (j + 1);
        }
    }
    integral /= wt->num_coefs;
    for (size_t i = 0; i < wt->num_coefs; i++) {
        size_t index = i * (wt->degree + 1);
        wt->coefs[index] -= integral;
    }
}

void wavetable_integrate(wavetable *wt)
{
    double *new_coefs = malloc(wt->num_coefs * (wt->degree + 2) * sizeof(double));
    double integral = 0;
    for (size_t i = 0; i < wt->num_coefs; i++) {
        size_t index = i * (wt->degree + 1);
        size_t index_new = i * (wt->degree + 2);
        new_coefs[index_new] = integral;
        for (size_t j = 0; j <= wt->degree; j++) {
            new_coefs[index_new + j + 1] = wt->coefs[index + j] / (j + 1);
            integral += wt->coefs[index + j] / (j + 1);
        }
    }
    free(wt->coefs);
    wt->degree += 1;
    wt->coefs = new_coefs;
}

double wavetable_eval(const wavetable wt, double x)
{
    double floor_x = floor(x);
    size_t index = (size_t) (floor_x - floor(floor_x / wt.num_coefs) * wt.num_coefs) * (wt.degree + 1);
    x -= floor_x;
    double v = 0;
    for (size_t i = wt.degree; i != (size_t)(-1); i--) {
        v *= x;
        v += wt.coefs[index + i];
    }
    return v;
}

void wavetable_destroy(wavetable *wt)
{
    free(wt->coefs);
    wt->coefs = NULL;
    wt->num_coefs = 0;
}
