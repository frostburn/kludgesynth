#include <limits.h>

#define LCG_MAX UINT_MAX
#define PINK_RANDOM_BITS (24)
#define PINK_SHIFT ((sizeof(int) * 8) - PINK_RANDOM_BITS)

typedef struct snow_state
{
    double mu;
    double y0;
    double y1;
    double y2;
    double y3;
} snow_state;

typedef struct snower_state
{
    double rate;
    double amplitude;
    double mul;
    snow_state state;
} snower_state;

typedef struct pink_state
{
    int num_rows;
    int *rows;
    int running_sum;
    int index;
    int index_mask;
    double normalizer;
} pink_state;

unsigned int lcg()
{
    static unsigned int lcg_seed = 22222;
    return lcg_seed = (lcg_seed * 196314165) + 907633515;
}

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

void pink_preinit(pink_state *p)
{
    p->num_rows = 0;
    p->rows = NULL;
}

void pink_init(pink_state *p, int num_rows)
{
    p->num_rows = num_rows;
    p->rows = calloc(num_rows, sizeof(int));
    p->index = 0;
    p->index_mask = (1 << (num_rows - 1)) - 1;
    p->running_sum = 0;
    p->normalizer = 1.0 / (num_rows * (1 << (PINK_RANDOM_BITS - 1)));
}

void pink_destroy(pink_state *p)
{
    int *rows = p->rows;
    pink_preinit(p);
    free(rows);
}

double pink_step(pink_state *p)
{
    if (!p->rows) {
        return 0;
    }
    int row_index = popcount(p->index);
    assert(row_index < p->num_rows);
    p->index = (p->index + 1) & p->index_mask;
    p->running_sum -= p->rows[row_index];
    int new_random = lcg();
    new_random >>= PINK_SHIFT;
    p->running_sum += p->rows[row_index] = new_random;
    return p->running_sum * p->normalizer;
}
