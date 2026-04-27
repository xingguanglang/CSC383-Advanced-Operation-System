/*
 * main.c - Entry point for the Process Scheduling Simulator
 * COEN 383 - Project 2
 *
 * Runs 6 algorithms (+ 2 aging variants) x 5 runs each,
 * prints per-run details and final averaged statistics.
 */

#include "process.h"

/* ─── Helper: accumulate Stats ─── */

static void accumulate_stats(Stats *acc, const Stats *s) {
    acc->avg_turnaround += s->avg_turnaround;
    acc->avg_waiting    += s->avg_waiting;
    acc->avg_response   += s->avg_response;
    acc->throughput     += s->throughput;
}

static void accumulate_hpf(HPFStats *acc, const HPFStats *hs) {
    for (int p = 1; p <= NUM_PRIORITIES; p++)
        accumulate_stats(&acc->per_priority[p], &hs->per_priority[p]);
    accumulate_stats(&acc->overall, &hs->overall);
}

static void average_stats(Stats *s, int n) {
    s->avg_turnaround /= n;
    s->avg_waiting    /= n;
    s->avg_response   /= n;
    s->throughput     /= n;
}

static void average_hpf(HPFStats *hs, int n) {
    for (int p = 1; p <= NUM_PRIORITIES; p++)
        average_stats(&hs->per_priority[p], n);
    average_stats(&hs->overall, n);
}

/* ─── Main ─── */

int main(void) {
    Stats    fcfs_avg = {0}, sjf_avg = {0}, srt_avg = {0}, rr_avg = {0}; 
    HPFStats hpf_np_avg, hpf_p_avg, hpf_np_aging_avg, hpf_p_aging_avg;
    /* initialize hpf memory space*/
    memset(&hpf_np_avg,       0, sizeof(HPFStats));
    memset(&hpf_p_avg,        0, sizeof(HPFStats));
    memset(&hpf_np_aging_avg, 0, sizeof(HPFStats));
    memset(&hpf_p_aging_avg,  0, sizeof(HPFStats));

    unsigned int seeds[NUM_RUNS] = {42, 123, 456, 789, 1024};

    for (int run = 0; run < NUM_RUNS; run++) {
        printf("============================================================\n");
        printf("                        RUN %d (seed=%u)\n", run + 1, seeds[run]);
        printf("============================================================\n");

        generate_processes(seeds[run]);

        /* ── FCFS ── */
        printf("\n--- FCFS ---\n");
        print_processes();
        int end = run_fcfs();
        print_timeline(end);
        Stats s = compute_stats(end);
        print_stats(&s);
        accumulate_stats(&fcfs_avg, &s);

        /* ── SJF ── */
        printf("\n--- SJF ---\n");
        end = run_sjf();
        print_timeline(end);
        s = compute_stats(end);
        print_stats(&s);
        accumulate_stats(&sjf_avg, &s);

        /* ── SRT ── */
        printf("\n--- SRT ---\n");
        end = run_srt();
        print_timeline(end);
        s = compute_stats(end);
        print_stats(&s);
        accumulate_stats(&srt_avg, &s);

        /* ── Round Robin ── */
        printf("\n--- Round Robin ---\n");
        end = run_rr();
        print_timeline(end);
        s = compute_stats(end);
        print_stats(&s);
        accumulate_stats(&rr_avg, &s);

        /* ── HPF Non-Preemptive ── */
        printf("\n--- HPF Non-Preemptive ---\n");
        end = run_hpf_np(0);
        print_timeline(end);
        HPFStats hs = compute_hpf_stats(end);
        print_hpf_stats(&hs);
        accumulate_hpf(&hpf_np_avg, &hs);

        /* ── HPF Preemptive ── */
        printf("\n--- HPF Preemptive ---\n");
        end = run_hpf_p(0);
        print_timeline(end);
        hs = compute_hpf_stats(end);
        print_hpf_stats(&hs);
        accumulate_hpf(&hpf_p_avg, &hs);

        /* ── HPF Non-Preemptive with Aging (Extra Credit) ── */
        printf("\n--- HPF Non-Preemptive with Aging ---\n");
        end = run_hpf_np(1);
        print_timeline(end);
        hs = compute_hpf_stats(end);
        print_hpf_stats(&hs);
        accumulate_hpf(&hpf_np_aging_avg, &hs);

        /* ── HPF Preemptive with Aging (Extra Credit) ── */
        printf("\n--- HPF Preemptive with Aging ---\n");
        end = run_hpf_p(1);
        print_timeline(end);
        hs = compute_hpf_stats(end);
        print_hpf_stats(&hs);
        accumulate_hpf(&hpf_p_aging_avg, &hs);
    }

    /* ── Final Averages ── */
    printf("\n\n============================================================\n");
    printf("          FINAL AVERAGES OVER %d RUNS\n", NUM_RUNS);
    printf("============================================================\n");

    printf("\n--- FCFS ---\n");
    average_stats(&fcfs_avg, NUM_RUNS);
    print_stats(&fcfs_avg);

    printf("\n--- SJF ---\n");
    average_stats(&sjf_avg, NUM_RUNS);
    print_stats(&sjf_avg);

    printf("\n--- SRT ---\n");
    average_stats(&srt_avg, NUM_RUNS);
    print_stats(&srt_avg);

    printf("\n--- Round Robin ---\n");
    average_stats(&rr_avg, NUM_RUNS);
    print_stats(&rr_avg);

    printf("\n--- HPF Non-Preemptive ---\n");
    average_hpf(&hpf_np_avg, NUM_RUNS);
    print_hpf_stats(&hpf_np_avg);

    printf("\n--- HPF Preemptive ---\n");
    average_hpf(&hpf_p_avg, NUM_RUNS);
    print_hpf_stats(&hpf_p_avg);

    printf("\n--- HPF Non-Preemptive with Aging (Extra Credit) ---\n");
    average_hpf(&hpf_np_aging_avg, NUM_RUNS);
    print_hpf_stats(&hpf_np_aging_avg);

    printf("\n--- HPF Preemptive with Aging (Extra Credit) ---\n");
    average_hpf(&hpf_p_aging_avg, NUM_RUNS);
    print_hpf_stats(&hpf_p_aging_avg);

    printf("\n============================================================\n");
    printf("                    SIMULATION COMPLETE\n");
    printf("============================================================\n");

    return 0;
}
