typedef struct snow_state
{
    double mu;
    double y0;
    double y1;
    double y2;
    double y3;
} snow_state;

void snow_init(snow_state *s)
{
    s->mu = 0;
    s->y0 = frand();
    s->y1 = frand();
    s->y2 = frand();
    s->y3 = frand();
}

void snow_step(snow_state *s, double rate)
{
    s->mu += rate;
    while (s->mu >= 1) {
        s->y3 = s->y2;
        s->y2 = s->y1;
        s->y1 = s->y0;
        s->y0 = frand();
        s->mu -= 1;
    }
}

double snow_linear(const snow_state s)
{
    return s.y1 + s.mu * (s.y0 - s.y1);
}

double snow_catmull(const snow_state s)
{
    return catmull_rom(s.mu, s.y3, s.y2, s.y1, s.y0);
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
    if (s->state.y0 * s->state.y1 > 0) {
        snow_step(&(s->state), s->rate * rate);
    }
    return s->state.y0 * s->amplitude;
}


double snower_step_1(snower_state *s, double rate)
{
    s->amplitude *= s->mul;
    snow_step(&(s->state), s->rate * rate);
    for (int i = 0; i < 4; i++) {
        if (s->state.y0 * s->state.y1 > 0) {
            snow_step(&(s->state), s->rate * rate);
        }
    }

    return snow_linear(s->state) * s->amplitude;
}
