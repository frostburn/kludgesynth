typedef struct envelope_state
{
    double attack;
    double decay;
    double sustain;
    double release;
    double mu;

    double iattack;
    double idecay;
    double irelease;

    double last_on_time;
    double last_off_time;
    double accum;
} envelope_state;

void envelope_init(envelope_state *e, double attack, double decay, double sustain, double release, double mu)
{
    e->attack = attack;
    e->decay = decay;
    e->sustain = sustain;
    e->release = release;
    e->mu = mu;

    e->iattack = 1.0 / attack;
    e->idecay = 1.0 / decay;
    e->irelease = 1.0 / release;

    e->last_on_time = -A_LOT;
    e->last_off_time = -A_LOT;
    e->accum = 0.0;
}

double _envelope_step(envelope_state *e, double v)
{
    return e->accum = v + e->mu * (e->accum - v);
}

double erf_env_step(envelope_state *e, double t, double on_time, double off_time) {
    double t_on = t - on_time;
    double t_off = t - off_time;
    double v = ferf(e->iattack * t_on);
    if (t_off > 0) {
        e->last_off_time = off_time;
        v *= fexp(-e->irelease * t_off);
    }
    else {
        double t_last_off = t - e->last_off_time;
        v += fexp(-e->irelease * t_last_off);
    }
    return _envelope_step(e, v);
}
