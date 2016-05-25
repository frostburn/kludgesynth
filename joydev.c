#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

#define JOY_BUFFER (64)

#define INIT_BUTTON (129)
#define INIT_AXIS (130)
#define JOY_BUTTON (1)
#define JOY_AXIS (2)
#define JOY_AXIS_MAX = (32767.0)
#define JOY_AXIS_NORM (3.051850947599719e-05)

typedef unsigned short int joy_clock_t;

typedef struct joy_description {
    joy_clock_t clock;
    int num_buttons;
    int num_axis;
} joy_description;

typedef struct joy_event {
    joy_clock_t clock;
    unsigned short code;
    signed short axis;
    unsigned char type;
    unsigned char num;
} joy_event;

static int joy_fd;

void print_joy_description(const joy_description d) {
    printf("joy_description(\n  clock=%d\n  num_buttons=%d\n  num_axis=%d\n)\n", d.clock, d.num_buttons, d.num_axis);
}

void print_joy_event(const joy_event e) {
    printf("joy_event(\n  clock=%d\n  code=%d\n  axis=%d\n  type=%d\n  num=%d\n)\n", e.clock, e.code, e.axis, e.type, e.num);
}


int init_joy(joy_description* joy_desc)
{
    joy_event ev[JOY_BUFFER];
    int num_bytes = read(joy_fd, ev, sizeof(joy_event) * JOY_BUFFER);
    if (num_bytes < 0) {
        printf("Joy read error during init.\n");
        exit(1);
    }
    else {
        joy_desc->clock = 0;
        joy_desc->num_buttons = 0;
        joy_desc->num_axis = 0;
        for (int i = 0; i < num_bytes / sizeof(joy_event); i++){
            if (i == 0){
                joy_desc->clock = ev[i].clock;
            }
            if (ev[i].type == INIT_BUTTON){
                joy_desc->num_buttons += 1;
            }
            else if (ev[i].type == INIT_AXIS){
                joy_desc->num_axis += 1;
            }
            else {
                printf("Unkown event type %d encountered during joy initialization.\n", ev[i].type);
            }
        }
    }
    return 0;
}

int open_joy(char *filename)
{
    if (filename == NULL) {
        filename = "/dev/input/js0";
    }

    joy_fd = open(filename, O_RDONLY);
    if (joy_fd > 0) {
        printf("Device open at %s\n", filename);
    }
    else {
        printf("Cannot open device at %s\n", filename);
        return 0;
    }
    #ifdef DEV_NONBLOCK
        int flags = fcntl(joy_fd, F_GETFL, 0);
        if (fcntl(joy_fd, F_SETFL, flags | O_NONBLOCK) < 0) {
            printf("Error setting non-blocking mode\n");
            return 0;
        }
    #endif
    return 1;
}

void close_joy()
{
    close(joy_fd);
}

int process_joy(void (process_joy_event)(const joy_event))
{
    joy_event ev[JOY_BUFFER];
    int num_bytes = read(joy_fd, ev, sizeof(joy_event) * JOY_BUFFER);
    if (num_bytes < 0) {
        #ifdef DEV_NONBLOCK
            return num_bytes;
        #else
            printf("Error reading joy events\n");
            exit(1);
        #endif
    }
    for (int i = 0; i < num_bytes / sizeof(joy_event); i++) {
        process_joy_event(ev[i]);
    }
    return num_bytes;
}

#ifndef MAIN
int main()
{
    open_joy(NULL);
    joy_description joy_desc;
    init_joy(&joy_desc);
    print_joy_description(joy_desc);
    while(1) {
        process_joy(print_joy_event);
    }
    close_joy();
    return 0;
}
#endif
