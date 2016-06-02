#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#define KB_BUFFER (64)
#define KB_SIMULTANEOUS (6)

#define KEY_RELEASED (0)
#define KEY_PRESSED (1)
#define FLAGS_CHANGED (2)
#define KB_UNDEFINED (255)

typedef struct raw_kb_event
{
    unsigned char flags;
    unsigned char reserved;
    unsigned char keys[KB_SIMULTANEOUS];
} raw_kb_event;

typedef struct kb_event
{
    unsigned char flags;
    unsigned char key;
    unsigned char type;
} kb_event;

static raw_kb_event last_event = {0, 0, {0, 0, 0, 0, 0, 0}};
static int kb_fd;

void print_raw_kb_event(const raw_kb_event e) {
    printf("raw_kb_event(\n  flags=%d\n  reserved=%d\n  keys=%d", e.flags, e.reserved, e.keys[0]);
    for (int i = 1; i < KB_SIMULTANEOUS; i++) {
        printf(", %d", e.keys[i]);
    }
    printf("\n)\n");
}

void print_kb_event(const kb_event e) {
    printf("kb_event(\n  flags=%d\n  key=%d\n  type=%d\n)\n", e.flags, e.key, e.type);
}

int process_kb(void (process_kb_event)(const kb_event)) {
    raw_kb_event ev[KB_BUFFER];
    int num_bytes = read(kb_fd, ev, sizeof(raw_kb_event) * KB_BUFFER);
    if (num_bytes < 0) {
        return num_bytes;
    }
    for (int i = 0; i < num_bytes / sizeof(raw_kb_event); i++) {
        raw_kb_event e = ev[i];
        if (e.keys[0] == 1 && e.keys[1] == 1 && e.keys[2] == 1 && e.keys[3] == 1 && e.keys[4] == 1 && e.keys[5] == 1) {
            continue;
        }
        kb_event event = {0, 0, KB_UNDEFINED};
        for (int j = 0; j < KB_SIMULTANEOUS; j++) {
            unsigned char last_key = last_event.keys[j];
            if (last_key) {
                int key_released = 1;
                for (int k = 0; k < KB_SIMULTANEOUS; k++) {
                    if (e.keys[k] == last_key) {
                        key_released = 0;
                    }
                }
                if (key_released) {
                     event = (kb_event) {e.flags, last_key, KEY_RELEASED};
                     process_kb_event(event);
                }
            }
        }
        for (int k = 0; k < KB_SIMULTANEOUS; k++) {
            unsigned char new_key = e.keys[k];
            if (new_key) {
                int key_pressed = 1;
                for (int j = 0; j < KB_SIMULTANEOUS; j++) {
                    if (last_event.keys[j] == new_key) {
                        key_pressed = 0;
                    }
                }
                if (key_pressed) {
                     event = (kb_event) {e.flags, new_key, KEY_PRESSED};
                     process_kb_event(event);
                }
            }
        }
        if (event.type == KB_UNDEFINED) {
            event = (kb_event) {e.flags, 0, FLAGS_CHANGED};
            process_kb_event(event);
        }
        last_event = e;
    }
    return num_bytes;
}

int init_kb(char *filename) {
    if (filename == NULL) {
        filename = "/dev/hidraw2";
    }

    kb_fd = open(filename, O_RDONLY);
    if (kb_fd == -1) {
        printf("Error: cannot open %s\n", filename);
        return 0;
    }
    else {
        printf("Device open at %s\n", filename);
    }
    #ifdef DEV_NONBLOCK
        int flags = fcntl(kb_fd, F_GETFL, 0);
        if (fcntl(kb_fd, F_SETFL, flags | O_NONBLOCK) < 0) {
            printf("Error setting non-blocking mode\n");
            return 0;
        }
    #endif

    return 1;
}

void close_kb()
{
    close(kb_fd);
}

#ifndef MAIN
int main()
{
    init_kb(NULL);
    while(1) {
        process_kb(print_kb_event);
    }
    close_kb();
    return 0;
}
#endif
