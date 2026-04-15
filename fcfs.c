/*
 * fcfs.c - First Come First Served (Non-preemptive)
 */

#include "process.h"

int run_fcfs(void) {
    reset_processes();
    int current = -1;
    int t = 0;

    while (1) {
        /* If no process running or current finished, pick next by arrival order */
        if (current == -1 || processes[current].finished) {
            current = -1;
            int earliest_arrival = INT_MAX;

            for (int i = 0; i < num_processes; i++) {
                if (processes[i].finished || processes[i].started) continue;
                if (processes[i].arrival_time > t) continue;
                if (t >= MAX_QUANTA) continue;  /* no new starts after quantum 99 */

                if (processes[i].arrival_time < earliest_arrival) {
                    earliest_arrival = processes[i].arrival_time;
                    current = i;
                }
            }

            if (current != -1) {
                processes[current].started    = 1;
                processes[current].start_time = t;
            }
        }

        /* Termination: past 100 quanta and nothing to run */
        if (t >= MAX_QUANTA && current == -1) break;

        /* Execute one quantum */
        if (current != -1) {
            timeline[t] = processes[current].name;
            processes[current].remaining_time--;
            if (processes[current].remaining_time <= 0) {
                processes[current].finished    = 1;
                processes[current].finish_time = t + 1;
                current = -1;   /* will pick next at loop top */
            }
        } else {
            timeline[t] = '-';  /* CPU idle */
        }
        t++;
    }

    timeline_len = t;
    return t;
}
