#include <complex.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef SAMPDELTA
#define SAMPDELTA (0.1)
#endif

#define DELTA_OMEGA (2 * M_PI * SAMPDELTA)

typedef struct twozero_state {
    double x1;
    double x2;
    double b0;
    double b1;
    double b2;
} twozero_state;

typedef struct polezero_state {
    double x1;
    double y1;
    double b0;
    double b1;
    double a1;
} polezero_state;

typedef struct biquad_state
{
    double x1;
    double x2;
    double y1;
    double y2;
    double b0;
    double b1;
    double b2;
    double a0;
    double a1;
    double a2;
} biquad_state;

typedef struct cpole_state
{
    complex double y1;
    complex double a1;
} cpole_state;

// Using cpole makes this design transientless with respect to parameter changes in contrast to biquad.
typedef struct filter_state
{
    twozero_state twozero;
    cpole_state cpole;
} filter_state;

typedef struct convolver_state {
    int num_samples;
    int index;
    double *samples;
    double *coefs;
} convolver_state;

void twozero_reset(twozero_state *t)
{
    t->x1 = 0;
    t->x2 = 0;
    t->b0 = 0;
    t->b1 = 0;
    t->b2 = 0;
}

double twozero_step(twozero_state *t, double sample)
{
    double y0 = t->b0 * sample + t->b1 * t->x1 + t->b2 * t->x2;
    t->x2 = t->x1;
    t->x1 = sample;
    return y0;
}

void polezero_reset(polezero_state *p)
{
    p->x1 = 0;
    p->y1 = 0;
    p->b0 = 0;
    p->b1 = 0;
    p->a1 = 0;
}

double polezero_step(polezero_state *p, double sample)
{
    double y0 = p->b0 * sample + p->b1 * p->x1 - p->a1 * p->y1;
    p->x1 = sample;
    p->y1 = y0;
    return y0;
}

void polezero_integrator(polezero_state *p, double gain, double leak_constant)
{
    p->b0 = 0.5 * gain * SAMPDELTA;
    p->b1 = 0.5 * gain * SAMPDELTA;
    p->a1 = -leak_constant;
}

void biquad_reset(biquad_state *b)
{
    b->x1 = 0;
    b->x2 = 0;
    b->y1 = 0;
    b->y2 = 0;
    b->b0 = 0;
    b->b1 = 0;
    b->b2 = 0;
    b->a0 = 0;
    b->a1 = 0;
    b->a2 = 0;
}

double biquad_step(biquad_state *b, double sample)
{
    double y0 = b->b0 * sample + b->b1 * b->x1 + b->b2 * b->x2 - b->a1 * b->y1 - b->a2 * b->y2;
    y0 /= b->a0;
    b->x2 = b->x1;
    b->x1 = sample;
    b->y2 = b->y1;
    b->y1 = y0;
    return y0;
}

void biquad_lpf(biquad_state *b, double freq, double Q)
{
    double w0 = freq * DELTA_OMEGA;
    double alpha = sin(w0) / (2 * Q);
    b->b0 = (1 - cos(w0))/2;
    b->b1 =  1 - cos(w0);
    b->b2 = (1 - cos(w0))/2;
    b->a0 =  1 + alpha;
    b->a1 = -2*cos(w0);
    b->a2 =  1 - alpha;
}

void cpole_reset(cpole_state *p)
{
    p->y1 = 0;
    p->a1 = 0;
}

complex double cpole_step(cpole_state *p, complex double sample)
{
    return p->y1 = sample - p->a1 * p->y1;
}

void filter_reset(filter_state *filter)
{
    twozero_reset(&filter->twozero);
    cpole_reset(&filter->cpole);
}

double filter_step(filter_state *filter, double sample)
{
    sample = twozero_step(&filter->twozero, sample);
    return cimag(cpole_step(&filter->cpole, sample));
}

void filter_resonator(filter_state *filter, double freq, double decay)
{
    filter->cpole.a1 = -cexp(SAMPDELTA * (2 * M_PI * I * freq - decay));
}


void filter_lpf(filter_state *filter, double freq, double decay)
{
    filter_resonator(filter, freq, decay);
    filter->twozero.b0 = filter->twozero.b2 = 0.25;
    filter->twozero.b1 = 0.5;
}

void filter_hpf(filter_state *filter, double freq, double decay)
{
    filter_resonator(filter, freq, decay);
    filter->twozero.b0 = filter->twozero.b2 = 0.25;
    filter->twozero.b1 = -0.5;
}

void filter_bpf(filter_state *filter, double freq, double decay)
{
    filter_resonator(filter, freq, decay);
    filter->twozero.b0 = 1;
    filter->twozero.b1 = 0;
    filter->twozero.b2 = -1;
}

void filter_notch(filter_state *filter, double freq, double decay)
{
    filter_resonator(filter, freq, decay);
    filter->twozero.b0 = 1;
    filter->twozero.b1 = -2 * cos(freq * DELTA_OMEGA);
    filter->twozero.b2 = 1;
}

void filter_apf(filter_state *filter, double freq, double decay)
{
    filter_resonator(filter, freq, decay);
    double r = creal(filter->cpole.a1);
    double i = cimag(filter->cpole.a1);
    double m = 1.0 / sin(freq * DELTA_OMEGA);
    filter->twozero.b0 = (r * r + i * i) * m;
    filter->twozero.b2 = m;
    filter->twozero.b1 = 2 * r * m;
}

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

#ifndef MAIN
int main()
{
    cpole_state p;
    cpole_reset(&p);
    p.a1 = -cexp(0.2 * I);
    complex double y = cpole_step(&p, 1);
    for (int i = 0; i < 40; i++) {
        printf("%g + (%g) * 1j,", creal(y), cimag(y));
        y = cpole_step(&p, 0);
        if (i > 25) {
            p.a1 = -cexp(0.7 * I);
        }
    }
    printf("\n");
    return 0;
}
#endif
