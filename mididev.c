#define MIDI_BUFFER (64)

typedef struct midi_event {
    unsigned char type;
    unsigned char pitch;
    unsigned char velocity;
} midi_event;

static int  midi_fd;

void print_midi_event(const midi_event e) {
    printf("midi_event(\n  type=%d\n  pitch=%d\n  velocity=%d\n)\n", e.type, e.pitch, e.velocity);
}

int process_midi(void (process_midi_event)(const midi_event)) {
    midi_event ev[MIDI_BUFFER];
    int num_bytes = read(midi_fd, ev, sizeof(midi_event) * MIDI_BUFFER);
    if (num_bytes < 0) {
        #ifdef DEV_NONBLOCK
            return num_bytes;
        #else
            printf("Error reading midi events\n");
            exit(1);
        #endif
    }
    for (int i = 0; i < ceil_div(num_bytes, sizeof(midi_event)); i++) {
        process_midi_event(ev[i]);
    }
    return num_bytes;
}

int init_midi(char *filename) {
    if (filename == NULL) {
        filename = "/dev/midi2";
    }

    midi_fd = open(filename, O_RDONLY);
    if (midi_fd == -1) {
        printf("Error: cannot open %s\n", filename);
        return 0;
    }
    else {
        printf("Device open at %s\n", filename);
    }
    #ifdef DEV_NONBLOCK
        int flags = fcntl(midi_fd, F_GETFL, 0);
        if (fcntl(midi_fd, F_SETFL, flags | O_NONBLOCK) < 0) {
            printf("Error setting non-blocking mode\n");
            return 0;
        }
    #endif

    return 1;
}
