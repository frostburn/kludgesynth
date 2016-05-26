typedef struct snow_state
{
    double mu;
    double x0;
    double x1;
    double x2;
    double x3;
} snow_state;

void snow_init(snow_state *s)
{
    s->x0 = frand();
    s->x1 = frand();
    s->x2 = frand();
    s->x3 = frand();
}

void snow_step(snow_state *s, double rate)
{
    s->mu += rate;
    while (s->mu >= 1) {
        s->x3 = s->x2;
        s->x2 = s->x1;
        s->x1 = s->x0;
        s->x0 = frand();
        s->mu -= 1;
    }
}

double snow_linear(const snow_state s)
{
    return s.x0 + s.mu * (s.x1 - s.x0);
}

typedef struct snower_state
{
    double rate;
    double amplitude;
    double mul;
    snow_state state;
} snower_state;

void snower_init(snower_state *s, double freq)
{
    snow_init(&(s->state));
    s->rate = freq * SAMPDELTA;
}

double snower_step_0(snower_state *s, double rate)
{
    s->amplitude *= s->mul;
    snow_step(&(s->state), s->rate * rate);
    if (s->state.x0 * s->state.x1 > 0) {
        snow_step(&(s->state), s->rate * rate);
    }
    return s->state.x0 * s->amplitude;
}


double snower_step_1(snower_state *s, double rate)
{
    s->amplitude *= s->mul;
    snow_step(&(s->state), s->rate * rate);
    for (int i = 0; i < 4; i++) {
        if (s->state.x0 * s->state.x1 > 0) {
            snow_step(&(s->state), s->rate * rate);
        }
    }

    return snow_linear(s->state) * s->amplitude;
}
