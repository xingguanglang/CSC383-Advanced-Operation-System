/* ============================================================
 * simulator.c - 模拟器引擎核心实现
 * 负责人: Member 5
 *
 * 模拟流程:
 *   时间步进 100 ms (REF_INTERVAL_MS)
 *   每个时间片做:
 *     1. 检查是否有进程到达, 若空闲页框 >= 4 则启动它 (装入 page-0)
 *     2. 对每个正在运行的进程, 生成一次内存引用
 *        - 若页面在内存: HIT, 更新统计
 *        - 否则:        MISS, 若有空闲页框直接装入,
 *                       否则按算法选 victim 换出
 *     3. 检查进程是否到期, 到期则释放其页框
 *
 * 局部性引用算法 (按项目规范):
 *   r = rand() % 11 (生成 0..10)
 *   若 0 <= r < 7: Δi ∈ {-1, 0, +1} (各 1/3 概率)
 *   若 7 <= r <=10: |Δi| >= 2, 即 j ∈ [0, i-2] ∪ [i+2, size-1]
 *   越界时按 wrap (i 在 [0, size-1] 之间循环)
 * ============================================================ */
#include "simulator.h"
#include "page_table.h"
#include "replacement.h"
#include "stats.h"

/* ---------- 局部性引用算法 ---------- */
int next_page_with_locality(int i, int size) {
    if (size <= 1) return 0;
    int r = rand() % 11;          /* 0..10 */
    int next;
    if (r < 7) {
        /* 70% 概率: i, i-1, i+1 */
        int delta = (rand() % 3) - 1;     /* -1, 0, +1 */
        next = i + delta;
        if (next < 0) next = size - 1;     /* wrap */
        if (next >= size) next = 0;
    } else {
        /* 30% 概率: |Δ| >= 2 */
        /* 候选区间: [0, i-2] ∪ [i+2, size-1] */
        int low_count  = (i - 2 >= 0) ? (i - 1) : 0;        /* 元素数 */
        int high_count = (i + 2 <= size - 1) ? (size - 1 - (i + 2) + 1) : 0;
        int total = low_count + high_count;
        if (total <= 0) {
            /* 进程太小放不下, 退化到任意页 */
            next = rand() % size;
        } else {
            int pick = rand() % total;
            if (pick < low_count) next = pick;          /* [0, i-2] */
            else                  next = (i + 2) + (pick - low_count);
        }
    }
    return next;
}

/* ---------- 链表深拷贝 ---------- */
Process *clone_workload(Process *src) {
    Process *head = NULL, *tail = NULL;
    while (src) {
        Process *n = (Process *)malloc(sizeof(Process));
        memcpy(n, src, sizeof(Process));
        n->is_running = false;
        n->is_done    = false;
        n->pages_resident = 0;
        n->current_page = 0;
        n->start_ms = -1;
        n->finish_ms = -1;
        n->next = NULL;
        if (!head) head = n;
        else       tail->next = n;
        tail = n;
        src = src->next;
    }
    return head;
}

/* 尝试启动 (swap-in) 队首到达进程 */
static void try_start_processes(Process **queue_head,
                                Process **running_head,
                                int now_ms,
                                AlgorithmType alg,
                                bool verbose,
                                RunStats *stats) {
    while (*queue_head &&
           (*queue_head)->arrival_ms <= now_ms &&
           free_frame_count() >= MIN_FREE_TO_RUN) {

        Process *p = *queue_head;
        *queue_head = p->next;
        p->next = NULL;

        /* 装入 page-0 */
        int frame = allocate_frame(p->pid, p->name, 0, now_ms);
        if (frame < 0) {                    /* 不应发生,前面已检 */
            p->next = *queue_head;
            *queue_head = p;
            break;
        }
        p->is_running     = true;
        p->pages_resident = 1;
        p->current_page   = 0;
        p->start_ms       = now_ms;
        p->finish_ms      = now_ms + p->duration_ms;
        stats->processes_swapped_in++;

        /* 加入运行链表 */
        p->next = *running_head;
        *running_head = p;

        if (verbose) {
            char mmap[TOTAL_FRAMES + 1];
            build_memory_map(mmap, sizeof(mmap));
            printf("[t=%6.2fs] %-4s ENTER  size=%2d dur=%.1fs map=%s\n",
                   now_ms / 1000.0, p->name, p->size_pages,
                   p->duration_ms / 1000.0, mmap);
        }
    }
    (void)alg;
}

/* 处理一次内存引用 */
static void do_reference(Process *p, int now_ms,
                         AlgorithmType alg, bool verbose,
                         RunStats *stats, int *ref_print_left) {
    int next_page = next_page_with_locality(p->current_page, p->size_pages);
    p->current_page = next_page;

    int frame = find_page(p->pid, next_page);
    bool hit  = (frame >= 0);
    int evicted_pid = -1, evicted_page = -1;
    char evicted_name[MAX_PROC_NAME] = "-";

    if (hit) {
        stats->hits++;
        g_frames[frame].last_used_ms = now_ms;
        g_frames[frame].use_count++;
    } else {
        stats->misses++;
        if (free_frame_count() > 0) {
            allocate_frame(p->pid, p->name, next_page, now_ms);
            p->pages_resident++;
        } else {
            int victim = select_victim(alg, now_ms);
            if (victim >= 0) {
                evicted_pid  = g_frames[victim].owner_pid;
                evicted_page = g_frames[victim].page_num;
                strncpy(evicted_name, g_frames[victim].owner_name, MAX_PROC_NAME);
                free_frame(victim);
                allocate_frame(p->pid, p->name, next_page, now_ms);
            }
        }
    }

    /* 详细打印前 PRINT_REF_LIMIT 次 */
    if (verbose && *ref_print_left > 0) {
        printf("  [t=%6.2fs] %-4s ref page %2d  %s",
               now_ms / 1000.0, p->name, next_page,
               hit ? "HIT " : "MISS");
        if (!hit && evicted_pid >= 0) {
            printf("  evicted=%s/page%d", evicted_name, evicted_page);
        }
        printf("\n");
        (*ref_print_left)--;
    }
}

/* 检查并清理已完成进程 */
static void reap_completed(Process **running_head,
                           int now_ms, bool verbose,
                           RunStats *stats) {
    Process **pp = running_head;
    while (*pp) {
        Process *p = *pp;
        if (now_ms >= p->finish_ms) {
            free_process_frames(p->pid);
            p->is_running = false;
            p->is_done    = true;
            stats->processes_completed++;

            if (verbose) {
                char mmap[TOTAL_FRAMES + 1];
                build_memory_map(mmap, sizeof(mmap));
                printf("[t=%6.2fs] %-4s EXIT   size=%2d                 map=%s\n",
                       now_ms / 1000.0, p->name, p->size_pages, mmap);
            }
            *pp = p->next;
            free(p);
        } else {
            pp = &(p->next);
        }
    }
}

/* ---------- 主模拟循环 ---------- */
RunStats run_simulation(AlgorithmType alg,
                        Process *workload_template,
                        bool verbose) {
    RunStats stats = {0, 0, 0, 0};
    init_frames();

    Process *queue   = clone_workload(workload_template);
    Process *running = NULL;
    int ref_print_left = verbose ? PRINT_REF_LIMIT : 0;
    int total_ms = SIM_DURATION_SEC * 1000;

    if (verbose) {
        printf("\n========== Run with algorithm: %s ==========\n",
               ALG_NAMES[alg]);
    }

    for (int now = 0; now <= total_ms; now += REF_INTERVAL_MS) {
        try_start_processes(&queue, &running, now, alg, verbose, &stats);

        for (Process *p = running; p; p = p->next) {
            if (p->is_running && now >= p->start_ms) {
                do_reference(p, now, alg, verbose, &stats, &ref_print_left);
            }
        }

        reap_completed(&running, now, verbose, &stats);
    }

    /* 清理 */
    while (running) { Process *n = running->next; free(running); running = n; }
    while (queue)   { Process *n = queue->next;   free(queue);   queue   = n; }

    return stats;
}
