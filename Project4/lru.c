/* ============================================================
 * lru.c - LRU (Least Recently Used) 页面替换算法
 * 负责人: Member 3
 *
 * 思路:
 *   每个页框记录 last_used_ms (最后访问时间).
 *   命中或装入时, simulator 会更新该字段.
 *   选 victim 时挑选 last_used_ms 最小的页框.
 * ============================================================ */
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
