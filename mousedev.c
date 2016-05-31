#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#define MOUSE_BUFFER (64)
#define MOUSE_CLICK_1 (4)
#define MOUSE_CLICK_2 (1)

typedef unsigned long long int mouse_clock_t;

typedef struct raw_mouse_event
{
    unsigned int timestamp;
    unsigned int reserved_1;
    unsigned int clock;
    unsigned int reserved_2;
    unsigned short type;
    unsigned short sub_type;
    signed int amount;
} raw_mouse_event;

typedef struct mouse_event
{
    mouse_clock_t clock;
    unsigned short type;
    unsigned short sub_type;
    int num;
    signed int amount;
    unsigned char flags;
} mouse_event;

static int mouse_fd;

void print_mouse_event(const mouse_event e) {
    printf("mouse_event(\n  clock=%llu\n  type=%d\n  sub_type=%d\n  num=%d\n  amount=%d\n  flags=%d\n)\n", e.clock, e.type, e.sub_type, e.num, e.amount, e.flags);
}

int process_mouse(void (process_mouse_event)(const mouse_event)) {
    raw_mouse_event ev[MOUSE_BUFFER];
    int num_bytes = read(mouse_fd, ev, sizeof(raw_mouse_event) * MOUSE_BUFFER);
    if (num_bytes < 0) {
        #ifdef DEV_NONBLOCK
            return num_bytes;
        #else
            printf("Error reading mouse events\n");
            exit(1);
        #endif
    }
    mouse_event e = {0, 0, 0, 0, 0, 0};
    for (int i = 0; i < num_bytes / sizeof(raw_mouse_event); i++) {
        if (ev[i].type) {
            mouse_clock_t clock = ev[i].timestamp;
            clock = (clock << 20) | ev[i].clock;
            unsigned char flags = (unsigned int) ev[i].amount;
            int type = ev[i].type;
            // Click events are merged at this level for easier processing.
            int num = flags;
            if (type == MOUSE_CLICK_2) {
                num = e.num;
            }
            e = (mouse_event) {clock, type, ev[i].sub_type, num, ev[i].amount, flags};
            if (type != MOUSE_CLICK_1) {
                process_mouse_event(e);
            }
        }
    }
    return num_bytes;
}

int init_mouse(char *filename) {
    if (filename == NULL) {
        filename = "/dev/input/event2";
    }

    mouse_fd = open(filename, O_RDONLY);
    if (mouse_fd == -1) {
        printf("Error: cannot open %s\n", filename);
        return 0;
    }
    else {
        printf("Device open at %s\n", filename);
    }
    #ifdef DEV_NONBLOCK
        int flags = fcntl(mouse_fd, F_GETFL, 0);
        if (fcntl(mouse_fd, F_SETFL, flags | O_NONBLOCK) < 0) {
            printf("Error setting non-blocking mode\n");
            return 0;
        }
    #endif

    return 1;
}

void close_mouse()
{
    close(mouse_fd);
}

#ifndef MAIN
int main()
{
    init_mouse(NULL);
    while(1) {
        process_mouse(print_mouse_event);
    }
    close_mouse();
    return 0;
}
#endif
