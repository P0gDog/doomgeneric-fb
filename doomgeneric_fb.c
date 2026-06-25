#include "doomgeneric.h"

#include "doomkeys.h"
#define DOOM_KEY_LEFTARROW  KEY_LEFTARROW
#define DOOM_KEY_RIGHTARROW KEY_RIGHTARROW
#define DOOM_KEY_UPARROW    KEY_UPARROW
#define DOOM_KEY_DOWNARROW  KEY_DOWNARROW
#define DOOM_KEY_FIRE       KEY_FIRE
#define DOOM_KEY_USE        KEY_USE
#define DOOM_KEY_RSHIFT     KEY_RSHIFT
#define DOOM_KEY_ESCAPE     KEY_ESCAPE
#define DOOM_KEY_ENTER      KEY_ENTER
#define DOOM_KEY_TAB        KEY_TAB
#define DOOM_KEY_MINUS      KEY_MINUS
#define DOOM_KEY_EQUALS     KEY_EQUALS

#include <linux/fb.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

static int fb_fd = -1;
static uint32_t *fb_mem = NULL;
static int fb_width, fb_height, fb_stride;

#define MAX_INPUT_DEVS 16
static int input_fds[MAX_INPUT_DEVS];
static int input_fd_count = 0;

#define KEY_QUEUE_SIZE 64
typedef struct { int pressed; unsigned char key; } KeyEvent;
static KeyEvent key_queue[KEY_QUEUE_SIZE];
static int key_queue_head = 0, key_queue_tail = 0;

static unsigned char linux_to_doom(unsigned short code) {
    switch (code) {
        case KEY_LEFT:      return DOOM_KEY_LEFTARROW;
        case KEY_RIGHT:     return DOOM_KEY_RIGHTARROW;
        case KEY_UP:        return DOOM_KEY_UPARROW;
        case KEY_DOWN:      return DOOM_KEY_DOWNARROW;
        case KEY_LEFTCTRL:
        case KEY_RIGHTCTRL: return DOOM_KEY_FIRE;
        case KEY_SPACE:     return DOOM_KEY_USE;
        case KEY_LEFTSHIFT:
        case KEY_RIGHTSHIFT:return DOOM_KEY_RSHIFT;
        case KEY_ESC:       return DOOM_KEY_ESCAPE;
        case KEY_ENTER:     return 13;
        case KEY_TAB:       return DOOM_KEY_TAB;
        case KEY_MINUS:     return DOOM_KEY_MINUS;
        case KEY_EQUAL:     return DOOM_KEY_EQUALS;
        case KEY_Y:         return 'y';
        case KEY_N:         return 'n';
        default:
            if (code >= KEY_A && code <= KEY_Z)
                return 'a' + (code - KEY_A);
            return 0;
    }
}

void DG_Init() {
    fb_fd = open("/dev/fb0", O_RDWR);
    if (fb_fd < 0) { perror("open /dev/fb0"); exit(1); }

    struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo finfo;
    if (ioctl(fb_fd, FBIOGET_VSCREENINFO, &vinfo) < 0 ||
        ioctl(fb_fd, FBIOGET_FSCREENINFO, &finfo) < 0) {
        perror("ioctl fb"); exit(1);
    }

    fb_width  = vinfo.xres;
    fb_height = vinfo.yres;
    fb_stride = finfo.line_length / 4;

    size_t fb_size = finfo.line_length * fb_height;
    fb_mem = mmap(NULL, fb_size, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, 0);
    if (fb_mem == MAP_FAILED) { perror("mmap fb"); exit(1); }

    // Open all available event devices
    for (int i = 0; i < MAX_INPUT_DEVS; i++) {
        char path[32];
        snprintf(path, sizeof(path), "/dev/input/event%d", i);
        int fd = open(path, O_RDONLY | O_NONBLOCK);
        if (fd >= 0) {
            input_fds[input_fd_count++] = fd;
        }
    }
}

void DG_DrawFrame() {
    uint32_t *src = DG_ScreenBuffer;
    for (int y = 0; y < fb_height; y++) {
        int sy = y * DOOMGENERIC_RESY / fb_height;
        for (int x = 0; x < fb_width; x++) {
            int sx = x * DOOMGENERIC_RESX / fb_width;
            fb_mem[y * fb_stride + x] = src[sy * DOOMGENERIC_RESX + sx];
        }
    }
}

void DG_SleepMs(uint32_t ms) {
    struct timespec ts = { ms / 1000, (ms % 1000) * 1000000L };
    nanosleep(&ts, NULL);
}

uint32_t DG_GetTicksMs() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

int DG_GetKey(int *pressed, unsigned char *doomKey) {
    // Poll all input devices
    for (int i = 0; i < input_fd_count; i++) {
        struct input_event ev;
        while (read(input_fds[i], &ev, sizeof(ev)) == sizeof(ev)) {
            if (ev.type == EV_KEY && ev.value != 2) {
                unsigned char dk = linux_to_doom(ev.code);
                if (dk != 0) {
                    int next = (key_queue_tail + 1) % KEY_QUEUE_SIZE;
                    if (next != key_queue_head) {
                        key_queue[key_queue_tail].pressed = ev.value ? 1 : 0;
                        key_queue[key_queue_tail].key = dk;
                        key_queue_tail = next;
                    }
                }
            }
        }
    }

    if (key_queue_head != key_queue_tail) {
        *pressed = key_queue[key_queue_head].pressed;
        *doomKey  = key_queue[key_queue_head].key;
        key_queue_head = (key_queue_head + 1) % KEY_QUEUE_SIZE;
        return 1;
    }
    return 0;
}

void DG_SetWindowTitle(const char *title) {}

int main(int argc, char **argv) {
    doomgeneric_Create(argc, argv);
    while (1) {
        doomgeneric_Tick();
    }
    return 0;
}
