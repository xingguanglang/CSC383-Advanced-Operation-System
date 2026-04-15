/*
 * stats.c - Statistics computation and printing
 */

#include "process.h"

/* ─── Compute overall stats for processes that actually ran ─── */

Stats compute_stats(int end_time) {
    Stats s = {0, 0, 0, 0};
    int count = 0;
    double total_tat = 0, total_wait = 0, total_resp = 0;

    for (int i = 0; i < num_processes; i++) {
        if (!processes[i].started) continue;
        count++;
        int tat  = processes[i].finish_time - processes[i].arrival_time;
        int wait = tat - processes[i].service_time;
        int resp = processes[i].start_time - processes[i].arrival_time;
        total_tat  += tat;
        total_wait += wait;
        total_resp += resp;
    }

    if (count > 0) {
        s.avg_turnaround = total_tat  / count;
        s.avg_waiting    = total_wait / count;
        s.avg_response   = total_resp / count;
        s.throughput     = (double)count / end_time;
    }
    return s;
}

/* ─── Compute per-priority and overall stats for HPF ─── */

HPFStats compute_hpf_stats(int end_time) {
    HPFStats hs;
    memset(&hs, 0, sizeof(hs));

    for (int p = 1; p <= NUM_PRIORITIES; p++) {
        int count = 0;
        double total_tat = 0, total_wait = 0, total_resp = 0;

        for (int i = 0; i < num_processes; i++) {
            if (!processes[i].started) continue;
            if (processes[i].original_priority != p) continue;
            count++;
            int tat  = processes[i].finish_time - processes[i].arrival_time;
            int wait = tat - processes[i].service_time;
            int resp = processes[i].start_time - processes[i].arrival_time;
            total_tat  += tat;
            total_wait += wait;
            total_resp += resp;
        }

        if (count > 0) {
            hs.per_priority[p].avg_turnaround = total_tat  / count;
            hs.per_priority[p].avg_waiting    = total_wait / count;
            hs.per_priority[p].avg_response   = total_resp / count;
            hs.per_priority[p].throughput     = (double)count / end_time;
        }
    }

    hs.overall = compute_stats(end_time);
    return hs;
}

/* ─── Print timeline chart ─── */

void print_timeline(int len) {
    printf("  Timeline: ");
    for (int i = 0; i < len; i++) {
        printf("%c", timeline[i]);
    }
    printf("\n");
}

/* ─── Print a Stats struct ─── */

void print_stats(Stats *s) {
    printf("  Avg Turnaround Time : %.2f\n", s->avg_turnaround);
    printf("  Avg Waiting Time    : %.2f\n", s->avg_waiting);
    printf("  Avg Response Time   : %.2f\n", s->avg_response);
    printf("  Throughput          : %.2f processes/quantum\n", s->throughput);
}

/* ─── Print HPF stats (per-priority + overall) ─── */

void print_hpf_stats(HPFStats *hs) {
    for (int p = 1; p <= NUM_PRIORITIES; p++) {
        printf("  --- Priority %d ---\n", p);
        print_stats(&hs->per_priority[p]);
    }
    printf("  --- Overall ---\n");
    print_stats(&hs->overall);
}
