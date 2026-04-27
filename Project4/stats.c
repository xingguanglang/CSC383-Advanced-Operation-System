/* ============================================================
 * stats.c - 统计输出实现
 * 负责人: Member 5
 * ============================================================ */
#include "stats.h"

void print_run_stats(AlgorithmType alg, int run_idx, RunStats s) {
    long total = s.hits + s.misses;
    double hit_ratio = (total > 0) ? (100.0 * s.hits / total) : 0.0;
    printf("  [%s][Run %d] hits=%ld misses=%ld total=%ld "
           "hit%%=%.2f swapped_in=%d completed=%d\n",
           ALG_NAMES[alg], run_idx + 1,
           s.hits, s.misses, total, hit_ratio,
           s.processes_swapped_in, s.processes_completed);
}

void print_alg_summary(AlgorithmType alg, RunStats runs[SIM_RUNS]) {
    long sum_h = 0, sum_m = 0;
    int  sum_si = 0, sum_done = 0;
    for (int i = 0; i < SIM_RUNS; i++) {
        sum_h    += runs[i].hits;
        sum_m    += runs[i].misses;
        sum_si   += runs[i].processes_swapped_in;
        sum_done += runs[i].processes_completed;
    }
    long total = sum_h + sum_m;
    double avg_hit = (total > 0) ? (100.0 * sum_h / total) : 0.0;
    double avg_miss = 100.0 - avg_hit;
    printf("  >> %s avg over %d runs: hit%%=%.2f miss%%=%.2f "
           "avg_swapped_in=%.1f avg_completed=%.1f\n",
           ALG_NAMES[alg], SIM_RUNS, avg_hit, avg_miss,
           sum_si / (double)SIM_RUNS, sum_done / (double)SIM_RUNS);
}

void print_overall_summary(RunStats results[ALG_COUNT][SIM_RUNS]) {
    printf("\n");
    printf("=================================================================\n");
    printf("                 OVERALL  SUMMARY  (5-run averages)              \n");
    printf("=================================================================\n");
    printf("%-8s | %-8s | %-8s | %-12s | %-12s\n",
           "Alg", "Hit%", "Miss%", "Swapped-In", "Completed");
    printf("---------+----------+----------+--------------+--------------\n");
    for (int a = 0; a < ALG_COUNT; a++) {
        long sum_h = 0, sum_m = 0;
        int  sum_si = 0, sum_done = 0;
        for (int i = 0; i < SIM_RUNS; i++) {
            sum_h    += results[a][i].hits;
            sum_m    += results[a][i].misses;
            sum_si   += results[a][i].processes_swapped_in;
            sum_done += results[a][i].processes_completed;
        }
        long t = sum_h + sum_m;
        double hit = (t > 0) ? (100.0 * sum_h / t) : 0.0;
        printf("%-8s | %7.2f%% | %7.2f%% | %12.1f | %12.1f\n",
               ALG_NAMES[a], hit, 100.0 - hit,
               sum_si / (double)SIM_RUNS,
               sum_done / (double)SIM_RUNS);
    }
    printf("=================================================================\n");
}
