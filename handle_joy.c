#define NUM_BUTTONS (32)

typedef struct joy_state_t
{
    double pitch_bend;
    double modulation;
    double velocity;
    double pitch_shift;
    double param_a;
    double param_b;
    int program_mode;
    int key_mod;
    int octave;
    int transpose;
} joy_state_t;

static pthread_t joy_thread;

static double joy_event_times[MAX_EVENTS];
static double joy_event_clocks[MAX_EVENTS];
static joy_clock_t last_joy_clock;
static int joy_event_index = -1;

int index_by_button[NUM_BUTTONS];

static joy_state_t joy_state = {
    0, 0, 0.5, 0, 0, 0, 0, 0
};

void handle_joy_event(const joy_event e)
{
    joy_clock_t joy_clock = e.clock;
    if (joy_event_index < 0) {
        last_joy_clock = joy_clock;
        joy_event_index = 0;
    }
    double event_t = next_t - last_t + t;
    event_t = get_event_time(
        event_t, next_t, 2 * next_t - last_t,
        joy_clock - last_joy_clock,
        MAX_EVENTS, &joy_event_index,
        joy_event_times, joy_event_clocks
    );
    #ifdef DEBUG
        print_joy_event(e);
        printf("%d: %g %g %g -> %g\n", e.clock, last_t, t, next_t, event_t);
    #endif
    if (e.type == JOY_BUTTON) {
        if (e.axis) {
            if (e.num == 10) {
                printf("Program mode on\n");
                joy_state.program_mode = 1;
                return;
            }
            else if (joy_state.program_mode) {
                program = e.num;
                printf("Selecting program %d\n", program);
                return;
            }
            int index = find_voice_index();
            index_by_button[e.num] = index;
            phases[index] = 0;
            freqs[index] = mtof(major_pentatonic(e.num, joy_state.key_mod) + 60 + 12 * joy_state.octave + joy_state.transpose + joy_state.pitch_shift);
            velocities[index] = joy_state.velocity;
            on_times[index] = event_t;
            off_times[index] = A_LOT;
            off_velocities[index] = 0;
        }
        else {
            if (e.num == 10) {
                printf("Program mode off\n");
                joy_state.program_mode = 0;
                return;
            }
            int index = index_by_button[e.num];
            if (index >= 0) {
                off_times[index] = event_t;
                off_velocities[index] = 0.5;
            }
        }
    }
    else if (e.type == JOY_AXIS){
        if (!joy_state.program_mode) {
            if (e.num == 0) {
                joy_state.pitch_bend = 1.0 * e.axis * JOY_AXIS_NORM;
            }
            else if (e.num == 1) {
                joy_state.modulation = 0.5 * e.axis * JOY_AXIS_NORM;
            }
            else if (e.num == 2) {
                joy_state.param_a = 0.5 + 0.5 * e.axis * JOY_AXIS_NORM;
            }
            else if (e.num == 3 && 0) {  // Broken on my machine. :(
                joy_state.pitch_shift = e.axis * JOY_AXIS_NORM;
                printf("%g\n", joy_state.pitch_shift);
            }
            else if (e.num == 4) {
                joy_state.velocity = 0.5 - 0.5 * e.axis * JOY_AXIS_NORM;
            }
            else if (e.num == 5) {
                joy_state.param_b = 0.5 + 0.5 * e.axis * JOY_AXIS_NORM;
            }
        }
        if (e.num == 6) {
            if (joy_state.program_mode) {
                joy_state.transpose += MAX(-1, MIN(1, e.axis));
                printf("Transpose %d\n", joy_state.transpose);
            }
            else {
                joy_state.key_mod = MAX(-1, MIN(1, e.axis));
            }
        }
        else if (e.num == 7) {
            if (joy_state.program_mode) {
                joy_state.octave -= MAX(-1, MIN(1, e.axis));
                printf("Octave %d\n", joy_state.octave);
            }
        }
    }
}

void* joy_thread_function(void *args)
{
    while (1) {
        process_joy(handle_joy_event);
    }
}

void launch_joy()
{
    for (int i = 0; i < NUM_BUTTONS; i++) {
        index_by_button[i] = -1;
    }
    int status = pthread_create(&joy_thread, NULL, joy_thread_function, NULL);
    if (status == -1) {
        printf("Error: unable to create joy input thread.\n");
        exit(1);
    }
}