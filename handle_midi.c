#define NUM_PITCHES (128)

#define NOTE_ON (0x90)
#define NOTE_OFF (0x80)
#define CONTROL_CHANGE (0xb0)
#define PROGRAM_CHANGE (0xc0)
#define PITCH_BEND (0xe0)
#define MIDI_SYSTEM (0xf0)

#define MODULATION (0x01)
#define ALL_SOUND_OFF (0x78)
#define RESET_ALL_CONTROLLERS (0x79)
#define ALL_NOTES_OFF (0x7b)  // 0x7b through 0x7f all imply all notes off.
#define OMNI_MODE_OFF (0x7c)
#define OMNI_MODE_ON (0x7d)
#define MONO_MODE_ON (0x7e)
#define POLY_MODE_ON (0x7f)

typedef struct midi_state_t
{
    double pitch_bend;
    double modulation;
} midi_state_t;

static pthread_t midi_thread;

static double midi_event_times[MAX_EVENTS];
static double midi_event_clocks[MAX_EVENTS];
static clock_t last_midi_clock;
static int midi_event_index = -1;

static int index_by_pitch[NUM_PITCHES];

static midi_state_t midi_state = {
    0, 0
};

void handle_midi_event(const midi_event e)
{
    clock_t midi_clock = clock();
    if (midi_event_index < 0) {
        last_midi_clock = midi_clock;
        midi_event_index = 0;
    }
    double event_t = next_t - last_t + t;
    event_t = get_event_time(
        event_t, next_t, 2 * next_t - last_t,
        midi_clock - last_midi_clock,
        MAX_EVENTS, &midi_event_index,
        midi_event_times, midi_event_clocks
    );
    #ifdef DEBUG
        print_midi_event(e);
        printf("%ld: %g, %g, %g -> %g\n", midi_clock, last_t, t, next_t, event_t);
    #endif
    if (e.type == NOTE_ON) {
        int index = index_by_pitch[e.pitch];
        if (index >= 0) {
            handle_note_off(index, event_t, e.velocity / 127.0);
        }
        index = find_voice_index();
        index_by_pitch[e.pitch] = index;
        handle_note_on(index, event_t, mtof(e.pitch), e.velocity / 127.0);
    }
    else if (e.type == NOTE_OFF) {
        int index = index_by_pitch[e.pitch];
        if (index >= 0) {
            handle_note_off(index, event_t, e.velocity / 127.0);
        }
    }
    else if (e.type == CONTROL_CHANGE) {
        if (e.pitch == MODULATION) {
            midi_state.modulation = e.velocity / 127.0;
        }
        else if (e.pitch == ALL_NOTES_OFF || e.pitch == ALL_SOUND_OFF) {
            for (int i = 0; i < NUM_VOICES; i++) {
                handle_note_off(i, event_t, 1);
            }
        }
    }
    else if (e.type == PITCH_BEND) {
        midi_state.pitch_bend = (e.velocity + e.pitch / 127.0 - 64.0) / 32.0;
        if (e.pitch == 127 && e.velocity == 127) {
            midi_state.pitch_bend = 2;
        }
    }
    else if (e.type == PROGRAM_CHANGE) {
        program = e.pitch;
        printf("Selecting program %d\n", program);
    }
}

void* midi_thread_function(void *args)
{
    while (1) {
        process_midi(handle_midi_event);
    }
}

void launch_midi()
{
    for (int i = 0; i < NUM_PITCHES; i++) {
        index_by_pitch[i] = -1;
    }
    int status = pthread_create(&midi_thread, NULL, midi_thread_function, NULL);
    if (status == -1) {
        printf("Error: unable to create midi input thread.\n");
        exit(1);
    }
}