/*
 * srt.c - Shortest Remaining Time (Preemptive)
 */

#include "process.h"

int run_srt(void) {
    reset_processes();
    int t = 0;

    while (1) {
        /* Each quantum: pick the process with shortest remaining time */
        int current = -1;
        int shortest = INT_MAX;

        for (int i = 0; i < num_processes; i++) {
            if (processes[i].finished) continue;
            if (processes[i].arrival_time > t) continue;
            if (!processes[i].started && t >= MAX_QUANTA) continue;

            if (processes[i].remaining_time < shortest) {
                shortest = processes[i].remaining_time;
                current = i;
            } else if (processes[i].remaining_time == shortest && current != -1 &&
                       processes[i].arrival_time < processes[current].arrival_time) {
                current = i;  /* tie-break by arrival time */
            }
        }

        if (t >= MAX_QUANTA && current == -1) break;

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
            }
        } else {
            timeline[t] = '-';
        }
        t++;
    }

    timeline_len = t;
    return t;
}
