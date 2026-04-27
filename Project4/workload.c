/* ============================================================
 * workload.c - 工作负载生成模块
 * 负责人: Member 1 (Team Lead)
 *
 * 实现细节:
 *   - 进程数量: NUM_PROCESSES (默认 150)
 *   - 进程大小: 在 {5, 11, 17, 31} MB 中随机均匀选取
 *   - 服务时长: 在 {1, 2, 3, 4, 5} 秒中随机均匀选取
 *   - 到达时间: 在 [0, SIM_DURATION_SEC] 区间内随机分布
 *   - 链表按到达时间升序排序
 * ============================================================ */
#include "workload.h"

/* 给进程分配名称: 前 26 个用 A-Z, 之后用 P27, P28 ... */
static void assign_name(Process *p, int idx) {
    if (idx < 26) {
        p->name[0] = 'A' + idx;
        p->name[1] = '\0';
    } else {
        snprintf(p->name, MAX_PROC_NAME, "P%02d", idx);
    }
}

/* 按 arrival_ms 升序插入链表 */
static Process *insert_sorted(Process *head, Process *node) {
    if (!head || node->arrival_ms < head->arrival_ms) {
        node->next = head;
        return node;
    }
    Process *cur = head;
    while (cur->next && cur->next->arrival_ms <= node->arrival_ms) {
        cur = cur->next;
    }
    node->next = cur->next;
    cur->next = node;
    return head;
}

Process *generate_workload(int count) {
    Process *head = NULL;
    int n_sizes = sizeof(PROC_SIZES) / sizeof(PROC_SIZES[0]);
    int n_durs  = sizeof(PROC_DURATIONS) / sizeof(PROC_DURATIONS[0]);

    for (int i = 0; i < count; i++) {
        Process *p = (Process *)calloc(1, sizeof(Process));
        if (!p) { perror("calloc"); exit(EXIT_FAILURE); }
        p->pid          = i;
        assign_name(p, i);
        p->size_pages   = PROC_SIZES[rand() % n_sizes];
        p->duration_ms  = PROC_DURATIONS[rand() % n_durs] * 1000;
        /* 到达时间在整个模拟窗口内均匀分布 */
        p->arrival_ms   = rand() % (SIM_DURATION_SEC * 1000);
        p->current_page = 0;
        p->is_running   = false;
        p->is_done      = false;
        p->pages_resident = 0;
        p->start_ms     = -1;
        p->finish_ms    = -1;
        p->next         = NULL;
        head = insert_sorted(head, p);
    }
    return head;
}

void free_workload(Process *head) {
    while (head) {
        Process *next = head->next;
        free(head);
        head = next;
    }
}

void print_workload(Process *head) {
    printf("=== Workload (%d processes) ===\n", NUM_PROCESSES);
    printf("%-6s %-6s %-6s %-10s %-10s\n",
           "PID", "Name", "Size", "Arrival(s)", "Duration(s)");
    int n = 0;
    for (Process *p = head; p && n < 20; p = p->next, n++) {
        printf("%-6d %-6s %-6d %-10.2f %-10.2f\n",
               p->pid, p->name, p->size_pages,
               p->arrival_ms / 1000.0, p->duration_ms / 1000.0);
    }
    if (n == 20) printf("... (truncated)\n");
}
