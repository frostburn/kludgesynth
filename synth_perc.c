#define NUM_PERC_PROGRAMS (3)

#define PERC_KICK (0)
#define PERC_SNARE (1)
#define PERC_HIHAT (2)

static PaStream *perc_stream;

double perc_t = 0;
double last_perc_t = 0;
double next_perc_t = 0;

static double perc_on_times[NUM_VOICES];
static double perc_off_times[NUM_VOICES];
static int perc_programs[NUM_VOICES];

static double perc_velocities[NUM_VOICES];
static double perc_off_velocities[NUM_VOICES];

static snare_state snares[NUM_VOICES];

void init_percussion()
{
    for (int i = 0; i < NUM_VOICES; i++) {
        perc_on_times[i] = -A_LOT;
        perc_off_times[i] = -A_LOT;
        perc_programs[i] = -1;

        perc_velocities[i] = 0;
        perc_off_velocities[i] = 0;

        snare_preinit(snares + i);
    }
}

int find_perc_index()
{
    double off_min = INFINITY;
    int index = 0;
    for (int i = 0; i < NUM_VOICES; i++) {
        if (perc_off_times[i] < off_min) {
            off_min = perc_off_times[i];
            index = i;
        }
    }
    if (off_min < t) {
        return index;
    }
    return -1;
}

void handle_perc_on(int index, int program, double event_t, double velocity)
{
    if (index < 0) {
        return;
    }
    perc_on_times[index] = event_t;
    perc_off_times[index] = A_LOT;
    perc_programs[index] = program;
    perc_velocities[index] = velocity;

    if (program == PERC_SNARE) {
        snare_destroy(snares + index);
        snare_init(snares + index, velocity);
    }
}

void handle_perc_off(int index, double event_t, double velocity)
{
    if (index < 0) {
        return;
    }
    int program = perc_programs[index];
    program += 0;
    if (event_t < perc_off_times[index]) {
        perc_off_times[index] = event_t;
        perc_off_velocities[index] = velocity;
    }
}

static int paPercCallback(
    const void *inputBuffer, void *outputBuffer,
    unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void *userData)
{
    paUserData *data = (paUserData*) userData;
    float *out = (float*) outputBuffer;
    double out_v;

    (void) timeInfo; /* Prevent unused variable warnings. */
    (void) statusFlags;
    (void) inputBuffer;
    (void) data;

    last_perc_t = perc_t;
    next_perc_t = perc_t + framesPerBuffer * SAMPDELTA;


    for (int i = 0; i < framesPerBuffer; i++) {
        out_v = 0;
        for (int j = 0; j < NUM_VOICES; j++) {
            int program = perc_programs[j];
            double t_on = perc_t - perc_on_times[j];
            double t_off = perc_t - perc_off_times[j];
            if (t_off > MAX_DECAY || t_on < 0) {
                continue;
            }
            double v = 0;
            if (program == PERC_KICK) {
                if (t_on < 0.5) {
                    v += ferf(cub(20 * exp(-10 * t_on)) * fexp(-5 * t_on)) * 0.2 * perc_velocities[j];
                }
            }
            else if (program == PERC_SNARE) {
                v += snare_step(snares + j, t_on);
            }
            else if (program == PERC_HIHAT) {
                if (t_on < 1) {
                    v += frand() * t_on * fexp(-60 * t_on) * 14 * perc_velocities[j];
                }
            }
            out_v += v;
        }

        perc_t += SAMPDELTA;

        if (data->record_index < data->record_num_samples) {
            data->record_samples[data->record_index++] = out_v;
        }

        *out++ = (float) out_v;    /* left */
        *out++ = (float) out_v;    /* right */
    }
    return paContinue;
}
