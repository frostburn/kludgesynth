// Interpolate unreliable event time with a more reliable clock.
double get_event_time(
    double event_t, double min_t, double max_t,
    double clock_delta,
    int num_events, int *event_index,
    double *event_times,
    double *event_clocks)
{
    int index = *event_index;
    if (index >= num_events) {
        for (int i = 0; i < num_events; i++) {
            event_times[i] = event_times[i + 1];
            event_clocks[i] = event_clocks[i + 1];
        }
        index--;
    }

    double event_clock;
    if (index == 0) {
        event_clock = 0;
    }
    else {
        event_clock = event_clocks[index - 1] + clock_delta;
    }

    event_times[index] = event_t;
    event_clocks[index] = event_clock;
    index++;

    double m, b;
    if (linreg(index, event_clocks, event_times, &m, &b, NULL)) {
        event_t = event_clock * m + b;
        event_t = MAX(min_t, MIN(max_t, event_t));
    }
    *event_index = index;
    return event_t;
}
