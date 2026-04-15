/*
 * process.c - Process generation, reset, and utility functions
 */

#include "process.h"

/* ─── Global variable definitions ─── */

Process processes[MAX_PROCESSES];
int     num_processes;
char    timeline[TIMELINE_SIZE];
int     timeline_len;

/* ─── Comparator for qsort ─── */

static int cmp_arrival(const void *a, const void *b) {
    const Process *pa = (const Process *)a;
    const Process *pb = (const Process *)b;
    if (pa->arrival_time != pb->arrival_time)
        return pa->arrival_time - pb->arrival_time;
    return pa->name - pb->name;
}

/* ─── Generate a set of simulated processes ─── */

void generate_processes(unsigned int seed) {
    srand(seed);
    /*
     * Generate ~50 processes to ensure CPU is rarely idle > 2 consecutive quanta.
     * Naming: A-Z (0-25) then a-z (26-51).
     */
    num_processes = 50;

    for (int i = 0; i < num_processes; i++) {
        processes[i].arrival_time = rand() % 100;
        processes[i].service_time = (rand() % 10) + 1;
        processes[i].priority     = (rand() % 4) + 1;

        if (i < 26)
            processes[i].name = 'A' + i;
        else
            processes[i].name = 'a' + (i - 26);
    }

    qsort(processes, num_processes, sizeof(Process), cmp_arrival);
}

/* ─── Reset all processes for a new algorithm run ─── */

void reset_processes(void) {
    for (int i = 0; i < num_processes; i++) {
        processes[i].remaining_time     = processes[i].service_time;
        processes[i].start_time         = -1;
        processes[i].finish_time        = -1;
        processes[i].started            = 0;
        processes[i].finished           = 0;
        processes[i].waited_at_level    = 0;
        processes[i].original_priority  = processes[i].priority;
        processes[i].effective_priority = processes[i].priority;
    }
    memset(timeline, '-', sizeof(timeline));
    timeline_len = 0;
}

/* ─── Print all created processes ─── */

void print_processes(void) {
    printf("  %-6s %-10s %-10s %-8s\n", "Name", "Arrival", "Runtime", "Priority");
    for (int i = 0; i < num_processes; i++) {
        printf("  %-6c %-10d %-10d %-8d\n",
               processes[i].name,
               processes[i].arrival_time,
               processes[i].service_time,
               processes[i].priority);
    }
}

/* ─── Check if all started processes are finished ─── */

int all_started_done(void) {
    for (int i = 0; i < num_processes; i++) {
        if (processes[i].started && !processes[i].finished)
            return 0;
    }
    return 1;
}
