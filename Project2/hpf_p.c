/*
 * hpf_p.c - Highest Priority First, Preemptive
 *           (RR with time slice = 1 within each priority queue)
 *           Optional aging support.
 */

#include "process.h"

int run_hpf_p(int aging) {
    reset_processes();
    int t = 0;

    /* Per-priority circular queues */
    int pq[NUM_PRIORITIES + 1][MAX_PROCESSES];
    int pq_front[NUM_PRIORITIES + 1];
    int pq_rear[NUM_PRIORITIES + 1];
    int pq_size[NUM_PRIORITIES + 1];
    int in_pq[MAX_PROCESSES];

    memset(pq_front, 0, sizeof(pq_front));
    memset(pq_rear,  0, sizeof(pq_rear));
    memset(pq_size,  0, sizeof(pq_size));
    memset(in_pq,    0, sizeof(in_pq));

    int last_run = -1;  /* process that just ran */

    while (1) {
        /* 1. Aging: bump priority of waiting processes */
        if (aging) {
            for (int i = 0; i < num_processes; i++) {
                if (processes[i].finished) continue;
                if (processes[i].arrival_time > t) continue;
                if (i == last_run) continue;
                if (!in_pq[i]) continue;

                processes[i].waited_at_level++;
                if (processes[i].waited_at_level >= AGING_THRESHOLD &&
                    processes[i].effective_priority > 1) {
                    processes[i].effective_priority--;
                    processes[i].waited_at_level = 0;
                    /* Process stays in its current queue position;
                       effective_priority is checked when rebuilding below.
                       For simplicity, we rebuild queues periodically. */
                }
            }

            /* Rebuild queues to reflect updated priorities */
            /* Save which processes are queued */
            int queued_list[MAX_PROCESSES];
            int queued_count = 0;
            for (int i = 0; i < num_processes; i++) {
                if (in_pq[i]) {
                    queued_list[queued_count++] = i;
                    in_pq[i] = 0;
                }
            }
            /* Clear all queues */
            for (int p = 1; p <= NUM_PRIORITIES; p++) {
                pq_front[p] = 0;
                pq_rear[p]  = 0;
                pq_size[p]  = 0;
            }
            /* Re-insert with correct priority */
            for (int q = 0; q < queued_count; q++) {
                int i = queued_list[q];
                int prio = processes[i].effective_priority;
                in_pq[i] = 1;
                pq[prio][pq_rear[prio]] = i;
                pq_rear[prio] = (pq_rear[prio] + 1) % MAX_PROCESSES;
                pq_size[prio]++;
            }
        }

        /* 2. Enqueue newly arrived processes */
        for (int i = 0; i < num_processes; i++) {
            if (in_pq[i] || processes[i].finished) continue;
            if (processes[i].arrival_time > t) continue;
            if (i == last_run) continue;
            if (!processes[i].started && t >= MAX_QUANTA) continue;

            int prio = aging ? processes[i].effective_priority
                             : processes[i].priority;
            in_pq[i] = 1;
            pq[prio][pq_rear[prio]] = i;
            pq_rear[prio] = (pq_rear[prio] + 1) % MAX_PROCESSES;
            pq_size[prio]++;
        }

        /* 3. Re-add last running process after new arrivals */
        if (last_run >= 0 && !processes[last_run].finished && !in_pq[last_run]) {
            int prio = aging ? processes[last_run].effective_priority
                             : processes[last_run].priority;
            in_pq[last_run] = 1;
            pq[prio][pq_rear[prio]] = last_run;
            pq_rear[prio] = (pq_rear[prio] + 1) % MAX_PROCESSES;
            pq_size[prio]++;
        }
        last_run = -1;

        /* 4. Pick from highest priority non-empty queue */
        int current = -1;
        for (int p = 1; p <= NUM_PRIORITIES; p++) {
            /* Skip finished processes at front */
            while (pq_size[p] > 0) {
                int idx = pq[p][pq_front[p]];
                if (processes[idx].finished) {
                    pq_front[p] = (pq_front[p] + 1) % MAX_PROCESSES;
                    pq_size[p]--;
                    in_pq[idx] = 0;
                } else {
                    break;
                }
            }
            if (pq_size[p] > 0) {
                current = pq[p][pq_front[p]];
                pq_front[p] = (pq_front[p] + 1) % MAX_PROCESSES;
                pq_size[p]--;
                in_pq[current] = 0;
                break;
            }
        }

        /* 5. Termination check */
        if (t >= MAX_QUANTA && current == -1) break;

        /* 6. Execute one quantum */
        if (current != -1) {
            if (!processes[current].started) {
                processes[current].started    = 1;
                processes[current].start_time = t;
            }
            timeline[t] = processes[current].name;
            processes[current].remaining_time--;

            if (processes[current].remaining_time <= 0) {
                processes[current].finished    = 1;
                processes[current].finish_time = t + 1;
            } else {
                last_run = current;
            }
        } else {
            timeline[t] = '-';
        }
        t++;
    }

    timeline_len = t;
    return t;
}
