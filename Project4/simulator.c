#include "simulator.h"
#include "page_table.h"
#include "replacement.h"
#include "stats.h"

int next_page_with_locality(int i, int size) {
    if (size <= 1) return 0;
    int r = rand() % 11;         
    int next;
    if (r < 7) {
        int delta = (rand() % 3) - 1;     /* -1, 0, +1 */
        next = i + delta;
        if (next < 0) next = size - 1;     /* wrap */
        if (next >= size) next = 0;
    } else {
        int low_count  = (i - 2 >= 0) ? (i - 1) : 0;  
        int high_count = (i + 2 <= size - 1) ? (size - 1 - (i + 2) + 1) : 0;
        int total = low_count + high_count;
        if (total <= 0) {
            next = rand() % size;
        } else {
            int pick = rand() % total;
            if (pick < low_count) next = pick;          /* [0, i-2] */
            else                  next = (i + 2) + (pick - low_count);
        }
    }
    return next;
}

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

        /* page-0 */
        int frame = allocate_frame(p->pid, p->name, 0, now_ms);
        if (frame < 0) {                  
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

    /* PRINT_REF_LIMIT */
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

    while (running) { Process *n = running->next; free(running); running = n; }
    while (queue)   { Process *n = queue->next;   free(queue);   queue   = n; }

    return stats;
}
