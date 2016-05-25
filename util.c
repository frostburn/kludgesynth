#define A_LOT (1e12)
#define TWO_ROOT_TWELVE (1.0594630943592953)

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

// Midi pitch to frequency
double mtof(double pitch)
{
    // Same as 440.0 * pow(2.0, (pitch - 69) / 12.0);
    return pow(TWO_ROOT_TWELVE, pitch + 36.37631656229583);
}

// Midi interval to frequency ratio
double itor(double interval)
{
    return pow(TWO_ROOT_TWELVE, interval);
}

size_t ceil_div(size_t x, size_t y)
{
    if (x == 0) {
        return 0;
    }
    return 1 + ((x - 1) / y);
}

double frand()
{
    return 2.0 * (rand() / (double) RAND_MAX) - 1.0;
}
