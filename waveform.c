#define TABLE_SIZE (4096)

static double SINE_TABLE[TABLE_SIZE + 1];

void waveform_init()
{
    for(int i = 0; i < TABLE_SIZE + 1; i++) {
        SINE_TABLE[i] = sin(2 * M_PI * i / ((double) TABLE_SIZE));
    }
}

double sine(double phase)
{
    phase -= floor(phase);
    phase *= TABLE_SIZE;
    int index = (int) phase;
    double mu = phase - index;
    return SINE_TABLE[index] + mu * (SINE_TABLE[index + 1] - SINE_TABLE[index]);
}

double cosine(double phase)
{
    return sine(phase + 0.25);
}

double softsaw(double phase, double sharpness)
{
    sharpness = clip(sharpness, EPSILON, 1.0 - EPSILON);
    return atan(sharpness * sine(phase) / (1.0 + sharpness * cosine(phase))) / asin(sharpness);
}


double lissajous12(double phase, double sharpness, double bias)
{
    sharpness = clip(sharpness, -1, 1);
    bias = clip(bias, EPSILON - 1, 1 - EPSILON);
    return atan2((1 + sharpness) * sine(phase), (1 - sharpness) * cosine(2 * phase + 0.5 * bias)) * 4.0 / (3.0 * M_PI);
}


double lissajous13(double phase, double sharpness, double bias)
{
    sharpness = clip(sharpness, -1, 1);
    bias = clip(bias, EPSILON - 1, 1 - EPSILON);
    double x = phase - floor(phase + 0.5);
    return atan2((1 + sharpness) * sine(1.5 * x), (1 - sharpness) * cosine(0.5 * x + bias / 3.0)) * 2.0 / M_PI + x + x;
}
