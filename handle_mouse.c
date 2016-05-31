#define MOUSE_CLICK (1)
#define MOUSE_MOVE (2)
#define MOUSE_FLAGS (3)

#define HORIZONTAL (0)
#define VERTICAL (1)
#define WHEEL (8)

typedef signed long long int mouse_location_t;

typedef struct mouse_state_t
{
    mouse_location_t x;
    mouse_location_t y;
    mouse_location_t wheel;
    unsigned char flags;
} mouse_state_t;

static pthread_t mouse_thread;

static double mouse_event_times[MAX_EVENTS];
static double mouse_event_clocks[MAX_EVENTS];
static mouse_clock_t last_mouse_clock;
static int mouse_event_index = -1;

static int index_by_pitch[NUM_PITCHES];

static mouse_state_t mouse_state = {
    0, 0, 0, 0
};

void handle_mouse_event(const mouse_event e)
{
    if (mouse_event_index < 0) {
        last_mouse_clock = e.clock;
        mouse_event_index = 0;
    }
    double event_t = next_t - last_t + t;
    event_t = get_event_time(
        event_t, next_t, 2 * next_t - last_t,
        e.clock - last_mouse_clock,
        MAX_EVENTS, &mouse_event_index,
        mouse_event_times, mouse_event_clocks
    );
    #ifdef DEBUG
        print_mouse_event(e);
        printf("%llu: %g, %g, %g -> %g\n", e.clock, last_t, t, next_t, event_t);
    #endif
    if (e.type == MOUSE_MOVE) {
        if (e.sub_type == HORIZONTAL) {
            mouse_state.x += e.amount;
        }
        if (e.sub_type == VERTICAL) {
            mouse_state.y += e.amount;
        }
        if (e.sub_type == WHEEL) {
            mouse_state.wheel += e.amount;
        }
    }
    else if (e.type == MOUSE_FLAGS) {
        mouse_state.flags = e.flags;
    }
    else if (e.type == MOUSE_CLICK) {
        if (e.amount) {
            handle_mouse_click(e.num, event_t);
        }
        else {
            handle_mouse_release(e.num, event_t);
        }
    }
}

void* mouse_thread_function(void *args)
{
    while (1) {
        process_mouse(handle_mouse_event);
    }
}

void launch_mouse()
{
    int status = pthread_create(&mouse_thread, NULL, mouse_thread_function, NULL);
    if (status == -1) {
        printf("Error: unable to create mouse input thread.\n");
        exit(1);
    }
}
