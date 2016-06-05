#define NOTE_OFF (0x80)
#define NOTE_ON (0x90)
#define AFTERTOUCH (0xa0)
#define CONTROL_CHANGE (0xb0)
#define PROGRAM_CHANGE (0xc0)
#define CHANNEL_PRESSURE (0xd0)
#define PITCH_BEND (0xe0)
#define MIDI_SYSTEM (0xf0)

#define MIDI_BUFFER (128)

typedef struct midi_event
{
    unsigned char channel;
    unsigned char type;
    unsigned char pitch;
    unsigned char velocity;
} midi_event;

void print_midi_event(const midi_event e) {
    printf("midi_event(\n  channel=%d\n  type=%d\n  pitch=%d\n  velocity=%d\n)\n", e.channel, e.type, e.pitch, e.velocity);
}

int process_midi(int midi_fd, unsigned char *channel_ptr, unsigned char *type_ptr, void (process_midi_event)(const midi_event)) {
    unsigned char buffer[MIDI_BUFFER];
    int num_bytes = read(midi_fd, buffer, sizeof(unsigned char) * MIDI_BUFFER);
    if (num_bytes < 0) {
        return num_bytes;
    }
    midi_event e = {0, 0, 0, 0};
    unsigned char channel = *channel_ptr;
    unsigned char type = *type_ptr;
    int i = 0;
    while (i < num_bytes) {
        unsigned char status = buffer[i];
        unsigned char message_channel = status & 0x0f;
        unsigned char message_type = status & 0xf0;
        if (message_type >= NOTE_OFF) {
            channel = message_channel;
            type = message_type;
            *channel_ptr = channel;
            *type_ptr = type;
            // TODO: Deal with the various system messages.
            if (type == MIDI_SYSTEM) {
                break;
            }
            i++;
        }
        unsigned char velocity = 0;
        unsigned char pitch = buffer[i++];
        if (type == NOTE_OFF || type == NOTE_ON || type == AFTERTOUCH || type == CONTROL_CHANGE || type == PITCH_BEND) {
            velocity = buffer[i++];
        }
        e = (midi_event) {channel, type, pitch, velocity};
        process_midi_event(e);
    }
    return num_bytes;
}

int init_midi(char *filename) {
    if (filename == NULL) {
        filename = "/dev/midi2";
    }

    int midi_fd = open(filename, O_RDONLY);
    if (midi_fd == -1) {
        printf("Error: cannot open %s\n", filename);
        return midi_fd;
    }
    else {
        printf("Device open at %s\n", filename);
    }
    #ifdef DEV_NONBLOCK
        int flags = fcntl(midi_fd, F_GETFL, 0);
        if (fcntl(midi_fd, F_SETFL, flags | O_NONBLOCK) < 0) {
            printf("Error setting non-blocking mode\n");
            return -1;
        }
    #endif

    return midi_fd;
}

void close_midi(int midi_fd)
{
    close(midi_fd);
}
