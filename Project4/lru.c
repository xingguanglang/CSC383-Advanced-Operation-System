#include "page_table.h"
#include "replacement.h"

int select_victim_lru(int now_ms) {
    (void)now_ms;
    int victim = -1;
    int min_t = 0x7FFFFFFF;
    for (int i = 0; i < TOTAL_FRAMES; i++) {
        if (g_frames[i].in_use && g_frames[i].last_used_ms < min_t) {
            min_t = g_frames[i].last_used_ms;
            victim = i;
        }
    }
    return victim;
}
