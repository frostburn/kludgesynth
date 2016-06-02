#define NUM_KEYS (256)

static pthread_t kb_thread;

static double kb_event_times[MAX_EVENTS];
static double kb_event_clocks[MAX_EVENTS];
static clock_t last_kb_clock;
static int kb_event_index = -1;

void handle_kb_event(const kb_event e)
{
    clock_t kb_clock = clock();
    if (kb_event_index < 0) {
        last_kb_clock = kb_clock;
        kb_event_index = 0;
    }
    double event_t = next_t - last_t + t;
    event_t = get_event_time(
        event_t, next_t, 2 * next_t - last_t,
        kb_clock - last_kb_clock,
        MAX_EVENTS, &kb_event_index,
        kb_event_times, kb_event_clocks
    );
    #ifdef DEBUG
        print_kb_event(e);
        printf("%ld: %g, %g, %g -> %g\n", kb_clock, last_t, t, next_t, event_t);
    #endif
    if (e.type == KEY_PRESSED) {
        handle_drum_on(e.key, event_t);
    }
    else if (e.type == KEY_RELEASED) {
        handle_drum_off(e.key, event_t);
    }
}

void* kb_thread_function(void *args)
{
    while (1) {
        process_kb(handle_kb_event);
    }
}

void launch_kb()
{
    int status = pthread_create(&kb_thread, NULL, kb_thread_function, NULL);
    if (status == -1) {
        printf("Error: unable to create kb input thread.\n");
        exit(1);
    }
}
