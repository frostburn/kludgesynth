typedef struct osc_state
{
    double phase;
    double freq;
    double velocity;
    double off_velocity;
} osc_state;

void osc_step(osc_state *osc, double rate)
{
    osc->phase += SAMPDELTA * osc->freq * rate;
}

double bowed_marimba(const osc_state osc, double t, double t_on, double t_off, double param_a, double param_b)
{
    double p = osc.phase * 2 * M_PI;
    double v = 0.4 * osc.velocity * sin(
        p +
        sin(p * 5) * (0.1 + 0.3 * osc.velocity) +
        0.1 * sin(p) +
        0.8 * param_a * sin(p * 3) +
        1.0 * param_b * sin(p * 2)
    ) * exp(-t_on * 0.02) * tanh(t_on * 200);
    if (t_off >= 0) {
        v *= exp(-t_off * 10);
    }
    return v;
}

double soft_ping(const osc_state osc, double t, double t_on, double t_off, double param_a, double param_b)
{
    double p = osc.phase * 2 * M_PI;
    double a = 0.01 + 2 * param_a;
    double b = param_b;
    double t_eff;
    if (t_off < 0) {
        t_eff = t_on * 4;
    }
    else {
        t_eff = (t_on - t_off) * 4 + t_off * 2;
    }
    double d = exp(-t_eff);
    a *= d;
    b *= d;
    return 0.4 * osc.velocity * tanh(sin(p + b * sin(p + 3 * t)) * a) / tanh(a) * d * tanh(t_on * (300 + 200 * osc.velocity));
}

double softsaw_bass(const osc_state osc, double t, double t_on, double t_off, double param_a, double param_b)
{
    double sharpness = exp(-t_on * 2) * 0.8 + 0.1 + 0.1 * osc.velocity;
    double p = osc.phase;
    double beat = 0.22 * t_on;
    p += 0.2 * param_a * sine(3 * p - t_on) + 0.45 * param_b * cosine(p + 0.2 * t_on);
    double v = 0.3 * osc.velocity * sharpness * (1 - 0.3 * MAX(param_a, param_b)) * (
        softsaw(p - beat, sharpness) + softsaw(p + beat, sharpness)
    );
    if (t_off >= 0) {
        v *= exp(-t_off * 10);
    }
    return v;
}
