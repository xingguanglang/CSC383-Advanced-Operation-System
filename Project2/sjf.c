/*
 * sjf.c - Shortest Job First (Non-preemptive)
 */

#include "process.h"

int run_sjf(void) {
    reset_processes();
    int current = -1;
    int t = 0;

    while (1) {
        /* If no process running or current finished, pick shortest job */
        if (current == -1 || processes[current].finished) {
            current = -1;
            int shortest = INT_MAX;

            for (int i = 0; i < num_processes; i++) {
                if (processes[i].finished || processes[i].started) continue;
                if (processes[i].arrival_time > t) continue;
                if (t >= MAX_QUANTA) continue;

                if (processes[i].service_time < shortest) {
                    shortest = processes[i].service_time;
                    current = i;
                } else if (processes[i].service_time == shortest && current != -1 &&
                           processes[i].arrival_time < processes[current].arrival_time) {
                    current = i;  /* tie-break by arrival time */
                }
            }

            if (current != -1) {
                processes[current].started    = 1;
                processes[current].start_time = t;
            }
        }

        if (t >= MAX_QUANTA && current == -1) break;

        if (current != -1) {
            timeline[t] = processes[current].name;
            processes[current].remaining_time--;
            if (processes[current].remaining_time <= 0) {
                processes[current].finished    = 1;
                processes[current].finish_time = t + 1;
                current = -1;
            }
        } else {
            timeline[t] = '-';
        }
        t++;
    }

    timeline_len = t;
    return t;
}
