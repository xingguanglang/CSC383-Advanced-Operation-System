/* ============================================================
 * common.h - 公共数据结构与常量定义
 * 负责人: Member 1 (Team Lead) — 团队共同维护
 * COEN 383 Project-4: Page Replacement Algorithms Simulator
 * ============================================================ */
#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>

/* ---------------- 模拟参数 ---------------- */
#define TOTAL_FRAMES        100      /* 主内存总页框数 (100 MB / 1 MB) */
#define NUM_PROCESSES       150      /* 总进程数 */
#define MIN_FREE_TO_RUN     4        /* 进程启动至少需要的空闲页框数 */
#define SIM_DURATION_SEC    60       /* 单次模拟时长 (秒) */
#define SIM_RUNS            5        /* 每个算法运行的次数 */
#define REF_INTERVAL_MS     100      /* 内存引用间隔 (毫秒) */
#define PRINT_REF_LIMIT     100      /* 详细打印的引用记录数 */
#define MAX_PROC_NAME       8

/* 进程大小与服务时长候选 */
static const int PROC_SIZES[]    = {5, 11, 17, 31};
static const int PROC_DURATIONS[] = {1, 2, 3, 4, 5};

/* ---------------- 算法枚举 ---------------- */
typedef enum {
    ALG_FIFO = 0,
    ALG_LRU,
    ALG_LFU,
    ALG_MFU,
    ALG_RANDOM,
    ALG_COUNT
} AlgorithmType;

static const char *ALG_NAMES[ALG_COUNT] = {
    "FIFO", "LRU", "LFU", "MFU", "RANDOM"
};

/* ---------------- 进程结构 ---------------- */
typedef struct Process {
    char  name[MAX_PROC_NAME];   /* 进程名 (例如 "A","B"...或 "P01") */
    int   pid;                   /* 进程编号 */
    int   size_pages;            /* 进程大小 (页数) */
    int   arrival_ms;            /* 到达时间 (毫秒) */
    int   duration_ms;           /* 服务时长 (毫秒) */
    int   start_ms;              /* 实际开始时间 */
    int   finish_ms;             /* 完成时间 */
    int   current_page;          /* 当前引用的页号 (用于局部性) */
    bool  is_running;            /* 是否正在运行 */
    bool  is_done;               /* 是否已完成 */
    int   pages_resident;        /* 当前驻留页数 */
    struct Process *next;
} Process;

/* ---------------- 页框结构 ---------------- */
typedef struct Frame {
    bool   in_use;               /* 是否被占用 */
    int    owner_pid;            /* 拥有者进程ID */
    char   owner_name[MAX_PROC_NAME];
    int    page_num;             /* 该页框存放的虚拟页号 */
    int    load_time_ms;         /* 装入时间 (FIFO 用) */
    int    last_used_ms;         /* 最后使用时间 (LRU 用) */
    int    use_count;            /* 使用次数 (LFU/MFU 用) */
} Frame;

/* ---------------- 全局统计 ---------------- */
typedef struct {
    long hits;
    long misses;
    int  processes_swapped_in;   /* 成功换入主存(开始运行)的进程数 */
    int  processes_completed;    /* 完成的进程数 */
} RunStats;

#endif /* COMMON_H */
