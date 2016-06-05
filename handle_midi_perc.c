static double midi_perc_event_times[MAX_EVENTS];
static double midi_perc_event_clocks[MAX_EVENTS];
static clock_t last_midi_perc_clock;
static int midi_perc_event_index = -1;

static int index_by_perc_pitch[NUM_PITCHES];

void handle_midi_perc_event(const midi_event e)
{
    clock_t midi_perc_clock = clock();
    if (midi_perc_event_index < 0) {
        last_midi_perc_clock = midi_perc_clock;
        midi_perc_event_index = 0;
    }
    double event_t = next_t - last_t + t;
    event_t = get_event_time(
        event_t, next_t, 2 * next_t - last_t,
        midi_perc_clock - last_midi_perc_clock,
        MAX_EVENTS, &midi_perc_event_index,
        midi_perc_event_times, midi_perc_event_clocks
    );
    #ifdef DEBUG
        print_midi_event(e);
        printf("%ld: %g, %g, %g -> %g\n", midi_perc_clock, last_t, t, next_t, event_t);
    #endif
    if (e.type == NOTE_ON) {
        int index = index_by_perc_pitch[e.pitch];
        if (index >= 0) {
            handle_perc_off(index, event_t, e.velocity / 127.0);
        }
        index = find_perc_index();
        index_by_perc_pitch[e.pitch] = index;
        handle_perc_on(index, e.pitch, event_t, e.velocity / 127.0);
    }
    else if (e.type == NOTE_OFF) {
        int index = index_by_perc_pitch[e.pitch];
        if (index >= 0) {
            handle_perc_off(index, event_t, e.velocity / 127.0);
        }
    }
}
