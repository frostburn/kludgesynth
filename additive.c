#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#ifndef SAMPDELTA
#define SAMPDELTA (0.1)
#endif

typedef struct sineping_state
{
    double y0;
    double y1;
    double a1;
    double a2;
} sineping_state;

// Generates amplitude * sin(2 * pi * (t * freq + phase)) that exponentially decays by target factor in 1 second.
void sineping_init(sineping_state *s, double phase, double freq, double amplitude, double decay)
{
    double d = pow(decay, SAMPDELTA);
    double omega = 2 * M_PI * freq * SAMPDELTA;
    double phi = 2 * M_PI * phase;

    s->a1 = 2 * d * cos(omega);
    s->a2 = d * d;

    s->y1 = amplitude * sin(phi - omega) / d;
    s->y0 = amplitude * sin(phi);
}

double sineping_step(sineping_state *s)
{
    double y = s->y0;
    s->y0 = s->a1 * s->y0 - s->a2 * s->y1;
    s->y1 = y;
    return y;
}

typedef struct pingsum_state
{
    int num_voices;
    sineping_state *pings;
    double mu;
    double z0;
    double z1;
} pingsum_state;

void pingsum_preinit(pingsum_state *p)
{
    p->num_voices = 0;
    p->pings = NULL;
}

void pingsum_init(pingsum_state *p, int num_voices)
{
    p->num_voices = num_voices;
    p->pings = malloc(num_voices * sizeof(sineping_state));
    assert(p->pings);
    p->mu = 0;
    p->z0 = 0;
    p->z1 = 0;
}

void pingsum_destroy(pingsum_state *p)
{
    sineping_state *pings = p->pings;
    pingsum_preinit(p);
    free(pings);
}

double _pingsum_step(pingsum_state *p)
{
    double v = 0;
    for (int i = 0; i < p->num_voices; i++) {
        v += sineping_step(p->pings + i);
    }
    return v;
}

double pingsum_step(pingsum_state *p, double rate)
{
    p->mu += rate;
    while (p->mu >= 1) {
        p->z0 = p->z1;
        p->z1 = _pingsum_step(p);
        p->mu -= 1;
    }
    return p->z0 + p->mu * (p->z1 - p->z0);
}

double cosine_sum_unsafe(double phase, int num_coefs, double *coefs)
{
    double c = cos(2 * M_PI * phase);
    double c2 = c + c;
    double bk = coefs[num_coefs - 1];
    double bk1 = coefs[num_coefs - 2] + c2 * bk;
    double temp;
    for (int i = num_coefs - 3; i > 0; i--) {
        temp = bk1;
        bk1 = coefs[i] + c2 * bk1 - bk;
        bk = temp;
    }
    return coefs[0] + c * bk1 - bk;
}

double cosine_sum(double phase, int num_coefs, double *coefs)
{
    if (num_coefs == 0) {
        return 0;
    }
    else if (num_coefs == 1) {
        return coefs[0];
    }
    else if (num_coefs == 2) {
        return coefs[0] + cos(2 * M_PI * phase) * coefs[1];
    }
    return cosine_sum_unsafe(phase, num_coefs, coefs);
}

double cis_sum_real(double phase, double sharpness, int num_coefs, double *coefs)
{
    phase *= 2 * M_PI;
    double x = cos(phase) * sharpness;
    double y = sin(phase) * sharpness;
    double result_x = coefs[num_coefs - 1];
    double result_y = 0;
    for (int i = num_coefs - 2; 1; i--) {
        phase = result_x;
        result_x = result_x * x - result_y * y;
        result_x += coefs[i];
        if (i <= 0) {
            return result_x;
        }
        result_y = phase * y + result_y * x;
    }
    return result_x;
}

#ifndef MAIN
int main()
{
    double coefs[] = {1, 3, 5, 6, 2};
    printf("%g %g\n", cosine_sum(0, 0, coefs), cosine_sum(0, 1, coefs));
    printf("%g = %g\n", 1 + 3 * cos(M_PI * 0.2), cosine_sum(0.1, 2, coefs));
    for (double p = 0; p < 1; p += 0.1) {
        double s = 0;
        for (int i = 0; i < 5; i++) {
            s += cos(2 * M_PI * p * i) * coefs[i];
        }
        printf("%g = %g\n", s, cosine_sum(p, 5, coefs));
    }
}
#endif
