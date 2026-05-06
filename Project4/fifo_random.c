#include "page_table.h"
#include "replacement.h"

int select_victim_fifo(int now_ms) {
    (void)now_ms;
    int victim = -1;
    int oldest = 0x7FFFFFFF;
    for (int i = 0; i < TOTAL_FRAMES; i++) {
        if (g_frames[i].in_use && g_frames[i].load_time_ms < oldest) {
            oldest = g_frames[i].load_time_ms;
            victim = i;
        }
    }
    return victim;
}

int select_victim_random(int now_ms) {
    (void)now_ms;
    int used[TOTAL_FRAMES];
    int n = 0;
    for (int i = 0; i < TOTAL_FRAMES; i++) {
        if (g_frames[i].in_use) used[n++] = i;
    }
    if (n == 0) return -1;
    return used[rand() % n];
}
