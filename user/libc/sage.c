#include "sage.h"
// char get_env(void) {
//     char ret;
//     __asm__ volatile("mv a7, %1\necall\nmv %0, a0" : "=r"(ret) : "r"(2) : "a0", "a7");
//     return ret;
// }

// void putchar(char c) {
//     __asm__ volatile("mv a7, %0\nmv a0, %1\necall" : : "r"(1), "r"(c) : "a0", "a7");
// }


int get_env(char *name, char *value) {
    int ret;
    // __asm__ volatile("mv a7, %1\necall\nmv %0, a0" : "=r"(ret) : "r"(6) : "a0", "a7");
    __asm__ volatile("mv a7, %1\nmv a0, %2\nmv a1, %3\necall\nmv %0, a0" : "=r"(ret) : "r"(6), "r"(name), "r"(value) : "a0", "a1", "a7");
    return ret;
}
int put_env(char *name, char *value) {
    int ret;
    // __asm__ volatile("mv a7, %1\necall\nmv %0, a0" : "=r"(ret) : "r"(7) : "a0", "a7");
    __asm__ volatile("mv a7, %1\nmv a0, %2\nmv a1, %3\necall\nmv %0, a0" : "=r"(ret) : "r"(7), "r"(name), "r"(value) : "a0", "a1", "a7");
    return ret;
}

int get_pid(void) {
    int ret;
    __asm__ volatile("mv a7, %1\necall\nmv %0, a0" : "=r"(ret) : "r"(8) : "a0", "a7");
    return ret;
}

int next_pid(int pid) {
    int ret;
    // __asm__ volatile("mv a7, %1\nmv a0, %2\necall\nmv %0, a0" : "=r"(ret) : "r"(9), "r"(pid) : "a0", "a7");

    __asm__ volatile("mv a7, %1\nmv a0, %2\n\necall\nmv %0, a0"
                     : "=r"(ret)
                     : "r"(9), "r"(pid)
                     : "a0", "a7");
    return ret;
}

int pid_get_env(int pid, char *name, char *value) {
    int ret;
    // __asm__ volatile("mv a7, %1\nmv a0, %2\necall\nmv %0, a0" : "=r"(ret) : "r"(10), "r"(pid) : "a0", "a7");
    __asm__ volatile("mv a7, %1\nmv a0, %2\nmv a1, %3\nmv a2, %4\necall\nmv %0, a0" : "=r"(ret) : "r"(10), "r"(pid), "r"(name), "r"(value) : "a0", "a1", "a2", "a7");
    return ret;
}

int pid_put_env(int pid, char *name, char *value) {
    int ret;
    // __asm__ volatile("mv a7, %1\nmv a0, %2\necall\nmv %0, a0" : "=r"(ret) : "r"(11), "r"(pid) : "a0", "a7");
    __asm__ volatile("mv a7, %1\nmv a0, %2\nmv a1, %3\nmv a2, %4\necall\nmv %0, a0" : "=r"(ret) : "r"(11), "r"(pid), "r"(name), "r"(value) : "a0", "a1", "a2", "a7");
    return ret;
}

int screen_draw_rect(Pixel *buf, Rectangle *rect, uint64_t x_scale, uint64_t y_scale) {
    int ret;
    // Pass the buf in a0, rect in a1, x_scale in a2, y_scale in a3
    __asm__ volatile("mv a7, %1\nmv a0, %2\nmv a1, %3\nmv a2, %4\nmv a3, %5\necall\nmv %0, a0" : "=r"(ret) : "r"(12), "r"(buf), "r"(rect), "r"(x_scale), "r"(y_scale) : "a0", "a1", "a2", "a3", "a7");
    return ret;
}
int screen_get_dims(Rectangle *rect) {
    int ret;
    // Pass the rect in a0
    __asm__ volatile("mv a7, %1\nmv a0, %2\necall\nmv %0, a0" : "=r"(ret) : "r"(13), "r"(rect) : "a0", "a7");
    return ret;
}

uint64_t get_time(void) {
    uint64_t ret;
    __asm__ volatile("mv a7, %1\necall\nmv %0, a0" : "=r"(ret) : "r"(14) : "a0", "a7");
    return ret;
}

// void screen_flush(void) {
//     // Set A0 to NULL
//     __asm__ volatile("mv a7, %0\nmv a0, %1\necall" : : "r"(15), "r"(0) : "a0", "a7");
// }

void screen_flush(Rectangle *rect) {
    __asm__ volatile("mv a7, %0\nmv a0, %1\necall" : : "r"(15), "r"(rect) : "a0", "a7");
}

int get_keyboard_event(VirtioInputEvent *event) {
    int ret;
    // Pass the event in a0
    __asm__ volatile("mv a7, %1\nmv a0, %2\necall\nmv %0, a0" : "=r"(ret) : "r"(16), "r"(event) : "a0", "a7");
    return ret;
}

int get_tablet_event(VirtioInputEvent *event) {
    int ret;
    // Pass the event in a0
    __asm__ volatile("mv a7, %1\nmv a0, %2\necall\nmv %0, a0" : "=r"(ret) : "r"(17), "r"(event) : "a0", "a7");
    return ret;
}

bool path_exists(const char *path) {
    bool ret;
    __asm__ volatile("mv a7, %1\nmv a0, %2\necall\nmv %0, a0" : "=r"(ret) : "r"(18), "r"(path) : "a0", "a7");
    return ret;
}

bool path_is_dir(const char *path) {
    bool ret;
    __asm__ volatile("mv a7, %1\nmv a0, %2\necall\nmv %0, a0" : "=r"(ret) : "r"(19), "r"(path) : "a0", "a7");
    return ret;
}

bool path_is_file(const char *path) {
    bool ret;
    __asm__ volatile("mv a7, %1\nmv a0, %2\necall\nmv %0, a0" : "=r"(ret) : "r"(20), "r"(path) : "a0", "a7");
    return ret;
}


int path_list_dir(const char *path, char *buf, int buf_size, bool return_full_path) {
    int ret;
    // Pass the path in a0, buf in a1, buf_size in a2, return_full_path in a3
    __asm__ volatile("mv a7, %1\nmv a0, %2\nmv a1, %3\nmv a2, %4\nmv a3, %5\necall\nmv %0, a0" : "=r"(ret) : "r"(21), "r"(path), "r"(buf), "r"(buf_size), "r"(return_full_path) : "a0", "a1", "a2", "a3", "a7");
}

int spawn_process(char *path) {
    int ret;
    // Pass the path in a0
    __asm__ volatile("mv a7, %1\nmv a0, %2\necall\nmv %0, a0" : "=r"(ret) : "r"(22), "r"(path) : "a0", "a7");
    return ret;
}

int read_file(const char *path, char *buf, int buf_size) {
    int ret;
    // Pass the path in a0, buf in a1, buf_size in a2
    __asm__ volatile("mv a7, %1\nmv a0, %2\nmv a1, %3\nmv a2, %4\necall\nmv %0, a0" : "=r"(ret) : "r"(23), "r"(path), "r"(buf), "r"(buf_size) : "a0", "a1", "a2", "a7");
    return ret;
}

int get_file_size(const char *path) {
    int ret;
    // Pass the path in a0
    __asm__ volatile("mv a7, %1\nmv a0, %2\necall\nmv %0, a0" : "=r"(ret) : "r"(24), "r"(path) : "a0", "a7");
    return ret;
}