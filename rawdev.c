#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#define BUFFER_SIZE (1024)

int main(int argc, char *argv[])
{
    int event_size = -1;
    if (argc == 2) {
        event_size = atoi(argv[1]);
    }
    char *filename = NULL;
    if (filename == NULL) {
        filename = "/dev/input/event2";
    }

    int fd = open(filename, O_RDONLY);
    if (fd > 0) {
        printf("Device open at %s\n", filename);
    }
    else {
        printf("Cannot open device at %s\n", filename);
        return 1;
    }

    unsigned char ev[BUFFER_SIZE];
    int num_bytes;
    while (1) {
        num_bytes = read(fd, ev, sizeof(unsigned char) * BUFFER_SIZE);
        if (event_size < 0) {
            printf("Got %d bytes.\n", num_bytes);
            for (int i = 0; i < num_bytes; i++) {
                printf("%3d: %3d\n", i, ev[i]);
            }
        }
        else {
            for (int i = 0; i < event_size; i++) {
                printf(" %2d|", i);
            }
            printf("\n");
            for (int j = 0; j < num_bytes / event_size; j++) {
                for (int i = 0; i < event_size; i++) {
                    printf("%3d ", ev[i + j * event_size]);
                }
                printf("\n");
            }
        }
    }

    close(fd);
    return 0;
}
