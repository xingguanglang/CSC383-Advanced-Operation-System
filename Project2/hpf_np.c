/*
 * hpf_np.c - Highest Priority First, Non-Preemptive
 *            (FCFS within each priority queue)
 *            Optional aging support.
 */

#include "process.h"

int run_hpf_np(int aging) {
    reset_processes();
    int current = -1;
    int t = 0;

    while (1) {
        /* Apply aging to waiting (not-yet-started) processes */
        if (aging) {
            for (int i = 0; i < num_processes; i++) {
                if (processes[i].finished || processes[i].started) continue;
                if (processes[i].arrival_time > t) continue;
                if (current != -1 && i == current) continue;

                processes[i].waited_at_level++;
                if (processes[i].waited_at_level >= AGING_THRESHOLD &&
                    processes[i].effective_priority > 1) {
                    processes[i].effective_priority--;
                    processes[i].waited_at_level = 0;
                }
            }
        }

        /* If no process running or current finished, pick next */
        if (current == -1 || processes[current].finished) {
            current = -1;
            int best_prio    = INT_MAX;
            int best_arrival = INT_MAX;

            for (int i = 0; i < num_processes; i++) {
                if (processes[i].finished || processes[i].started) continue;
                if (processes[i].arrival_time > t) continue;
                if (t >= MAX_QUANTA) continue;

                int prio = aging ? processes[i].effective_priority
                                 : processes[i].priority;

                if (prio < best_prio ||
                    (prio == best_prio && processes[i].arrival_time < best_arrival)) {
                    best_prio    = prio;
                    best_arrival = processes[i].arrival_time;
                    current      = i;
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
