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

double fm_meow(const osc_state osc, double param_a, double param_b, double mod_a, double mod_b)
{
    double v = qui(osc.phase);
    v -= 2 * qua(osc.phase) * mod_b;
    v = qui(
        2 * osc.phase +
        qui(osc.phase + v * (0.07 + 0.13 * mod_a)) * param_a * 0.3 +
        v * (0.1 + param_b * 0.2)
    ) * (1 - 0.2 * param_a - 0.1 * param_b);
    return v * 0.5 * osc.velocity;
}

double formant_voice(const osc_state osc, double t, double rate, double param_a, double param_b, double mod_a, double mod_b)
{
    param_a -= mod_b * 0.9 * param_a;
    param_b -= mod_b * 0.9 * param_b;
    mod_b = 1.0 - mod_b * 0.8;

    param_a += mod_a * (-0.1 - 0.95 * param_a);
    param_b += mod_a * (0.4 - 0.8 * param_b);

    double v = 0;
    double ifreq = 0.95 / (osc.freq * rate);
    double width = 0.5 * pow(ifreq, 1.4);
    param_a -= param_b * 0.1;
    param_b -= param_a * 0.1;
    width *= (1 + 0.1 * param_a + 0.1 * param_b);
    width *= (1 + mod_a);
    v += formant(osc.phase * 1.000, 0.8, 4000 * width) * (0.2 + 0.1 * ifreq) * (0.4 * param_a + 0.5 * param_b + 3 * (1 - mod_b));
    v += formant(osc.phase * 1.000, (450 +  500 * param_a) * ifreq, (1800 + 5000 * param_a + 800 * param_b) * width) * 2 * (1 + 0.7 * param_a - 0.2 * param_b) * mod_b;
    v += formant(osc.phase * 1.001, (900 +  100 * param_a + 900 * param_b) * ifreq, (1800 + 2000 * param_a - 100 * param_b) * width) * (0.4 + 1.2 * param_b - 0.1 * param_a) * mod_b;
    v += formant(osc.phase * 0.999, (3000 - 200 * param_a - 500 * param_b + 700 * mod_a) * ifreq, (6000 + 3000 * param_a - 200 * param_b + 4000 * mod_a) * width) * 0.2 * (1 + 0.5 * param_b - 0.7 * param_a - 0.4 * mod_a) * mod_b;
    v += formant(osc.phase * 1.000, (3500 - 100 * param_a + 100 * param_b) * ifreq, (7000 - 200 * param_b + 200 * mod_a) * width) * 0.07 * mod_b;
    v *= (1 - 0.1 * param_a - 0.2 * param_b) * (0.9 + 0.1 * sine(t * 6.11 + 0.1 * cosine(t * 0.41)));
    return v * osc.velocity * 0.15;
}
