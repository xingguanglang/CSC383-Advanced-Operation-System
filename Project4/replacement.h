/* ============================================================
 * replacement.h - 页面替换算法接口
 *
 * 每种算法实现一个 select_victim 函数,
 * 返回需要被换出的页框索引 (0 ~ TOTAL_FRAMES-1)
 *
 * 负责人分工:
 *   - FIFO   : Member 2
 *   - RANDOM : Member 2
 *   - LRU    : Member 3
 *   - LFU    : Member 4
 *   - MFU    : Member 4
 * ============================================================ */
#ifndef REPLACEMENT_H
#define REPLACEMENT_H

#include "common.h"

/* 选出 victim 页框索引 */
int select_victim_fifo(int now_ms);
int select_victim_lru(int now_ms);
int select_victim_lfu(int now_ms);
int select_victim_mfu(int now_ms);
int select_victim_random(int now_ms);

/* 统一调度入口 */
int select_victim(AlgorithmType alg, int now_ms);

#endif
