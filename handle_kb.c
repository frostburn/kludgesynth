#define NUM_KEYS (256)

int index_by_key[NUM_KEYS];

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
    double event_t = next_perc_t - last_perc_t + perc_t;
    event_t = get_event_time(
        event_t, next_perc_t, 2 * next_perc_t - last_perc_t,
        kb_clock - last_kb_clock,
        MAX_EVENTS, &kb_event_index,
        kb_event_times, kb_event_clocks
    );
    #ifdef DEBUG
        print_kb_event(e);
        printf("%ld: %g, %g, %g -> %g\n", kb_clock, last_perc_t, perc_t, next_perc_t, event_t);
    #endif
    if (e.type == KEY_PRESSED) {
        int index = find_perc_index();
        index_by_key[e.key] = index;
        handle_perc_on(index, e.key % 2, event_t, 0.7);
    }
    else if (e.type == KEY_RELEASED) {
        int index = index_by_key[e.key];
        if (index >= 0) {
            handle_perc_off(index, event_t, 0.5);
        }
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
    for (int i = 0; i < NUM_KEYS; i++) {
        index_by_key[i] = -1;
    }
    int status = pthread_create(&kb_thread, NULL, kb_thread_function, NULL);
    if (status == -1) {
        printf("Error: unable to create kb input thread.\n");
        exit(1);
    }
}
