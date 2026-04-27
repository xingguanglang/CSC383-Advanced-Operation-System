/* ============================================================
 * stats.h - 统计输出模块
 * 负责人: Member 5
 * ============================================================ */
#ifndef STATS_H
#define STATS_H

#include "common.h"

/* 打印单次运行的统计 */
void print_run_stats(AlgorithmType alg, int run_idx, RunStats s);

/* 打印某算法 5 次运行的平均结果 */
void print_alg_summary(AlgorithmType alg, RunStats runs[SIM_RUNS]);

/* 打印总体对比表 */
void print_overall_summary(RunStats results[ALG_COUNT][SIM_RUNS]);

#endif
