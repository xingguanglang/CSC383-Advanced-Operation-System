/* ============================================================
 * page_table.h - 页框/内存管理模块
 * 负责人: Member 2
 *
 * 提供:
 *   - 全局页框数组 frames[TOTAL_FRAMES]
 *   - 查找进程某页是否在内存
 *   - 分配/释放页框
 *   - 输出 100 个页框的内存映射字符串 (用于日志)
 * ============================================================ */
#ifndef PAGE_TABLE_H
#define PAGE_TABLE_H

#include "common.h"

/* 全局页框数组 */
extern Frame g_frames[TOTAL_FRAMES];

/* 初始化所有页框为空闲 */
void init_frames(void);

/* 返回当前空闲页框数 */
int  free_frame_count(void);

/* 查找指定进程的某虚拟页是否在内存; 返回页框索引, 否则 -1 */
int  find_page(int pid, int page_num);

/* 分配一个空闲页框给 (pid, page_num); 返回页框索引, 否则 -1 */
int  allocate_frame(int pid, const char *pname, int page_num, int now_ms);

/* 释放指定页框 */
void free_frame(int frame_idx);

/* 释放某进程占用的所有页框 (进程结束时调用) */
void free_process_frames(int pid);

/* 生成内存映射字符串, 例如 "AAbbb..CCC..."  */
void build_memory_map(char *out, int out_size);

#endif
