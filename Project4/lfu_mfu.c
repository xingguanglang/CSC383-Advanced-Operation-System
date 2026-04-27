/* ============================================================
 * lfu_mfu.c - LFU (Least Frequently Used) 与
 *             MFU (Most Frequently Used) 页面替换算法
 * 负责人: Member 4
 *
 * 思路:
 *   每个页框维护 use_count (引用次数).
 *   命中或装入时由 simulator 自增.
 *   LFU: 挑选 use_count 最小的页框 (并列时取较旧的).
 *   MFU: 挑选 use_count 最大的页框 (并列时取较旧的).
 * ============================================================ */
#include "page_table.h"
#include "replacement.h"

int select_victim_lfu(int now_ms) {
    (void)now_ms;
    int victim = -1;
    int min_cnt = 0x7FFFFFFF;
    int oldest  = 0x7FFFFFFF;
    for (int i = 0; i < TOTAL_FRAMES; i++) {
        if (!g_frames[i].in_use) continue;
        if (g_frames[i].use_count < min_cnt ||
           (g_frames[i].use_count == min_cnt &&
            g_frames[i].load_time_ms < oldest)) {
            min_cnt = g_frames[i].use_count;
            oldest  = g_frames[i].load_time_ms;
            victim  = i;
        }
    }
    return victim;
}

int select_victim_mfu(int now_ms) {
    (void)now_ms;
    int victim = -1;
    int max_cnt = -1;
    int oldest  = 0x7FFFFFFF;
    for (int i = 0; i < TOTAL_FRAMES; i++) {
        if (!g_frames[i].in_use) continue;
        if (g_frames[i].use_count > max_cnt ||
           (g_frames[i].use_count == max_cnt &&
            g_frames[i].load_time_ms < oldest)) {
            max_cnt = g_frames[i].use_count;
            oldest  = g_frames[i].load_time_ms;
            victim  = i;
        }
    }
    return victim;
}

/* 统一调度入口 (放在这里,因为它需要看到所有算法函数原型) */
int select_victim(AlgorithmType alg, int now_ms) {
    switch (alg) {
        case ALG_FIFO:   return select_victim_fifo(now_ms);
        case ALG_LRU:    return select_victim_lru(now_ms);
        case ALG_LFU:    return select_victim_lfu(now_ms);
        case ALG_MFU:    return select_victim_mfu(now_ms);
        case ALG_RANDOM: return select_victim_random(now_ms);
        default:         return -1;
    }
}
