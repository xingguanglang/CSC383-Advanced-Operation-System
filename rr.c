/*
 * rr.c - Round Robin (Preemptive, time slice = 1 quantum)
 */

#include "process.h"

int run_rr(void) {
    reset_processes();
    int t = 0;

    /* Circular ready queue */
    int queue[MAX_PROCESSES];
    int q_front = 0, q_rear = 0, q_size = 0;
    int in_queue[MAX_PROCESSES];
    memset(in_queue, 0, sizeof(in_queue));

    int last_run = -1;  /* process that just ran; re-added after new arrivals */

    while (1) {
        /* 1. Enqueue newly arrived processes */
        for (int i = 0; i < num_processes; i++) {
            if (in_queue[i] || processes[i].finished) continue;
            if (processes[i].arrival_time > t) continue;
            if (!processes[i].started && t >= MAX_QUANTA) continue;
            if (i == last_run) continue;  /* handled separately below */

            in_queue[i] = 1;
            queue[q_rear] = i;
            q_rear = (q_rear + 1) % MAX_PROCESSES;
            q_size++;
        }

        /* 2. Re-add last running process AFTER new arrivals (RR fairness) */
        if (last_run >= 0 && !processes[last_run].finished && !in_queue[last_run]) {
            in_queue[last_run] = 1;
            queue[q_rear] = last_run;
            q_rear = (q_rear + 1) % MAX_PROCESSES;
            q_size++;
        }
        last_run = -1;

        /* 3. If queue empty, idle or terminate */
        if (q_size == 0) {
            if (t >= MAX_QUANTA) break;
            timeline[t] = '-';
            t++;
            continue;
        }

        /* 4. Dequeue front process */
        int current = queue[q_front];
        q_front = (q_front + 1) % MAX_PROCESSES;
        q_size--;
        in_queue[current] = 0;

        if (!processes[current].started) {
            processes[current].started    = 1;
            processes[current].start_time = t;
        }

        /* 5. Execute one quantum */
        timeline[t] = processes[current].name;
        processes[current].remaining_time--;

        if (processes[current].remaining_time <= 0) {
            processes[current].finished    = 1;
            processes[current].finish_time = t + 1;
        } else {
            last_run = current;  /* will be re-added next iteration */
        }
        t++;
    }

    timeline_len = t;
    return t;
}
