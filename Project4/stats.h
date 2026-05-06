#ifndef STATS_H
#define STATS_H

#include "common.h"

void print_run_stats(AlgorithmType alg, int run_idx, RunStats s);

void print_alg_summary(AlgorithmType alg, RunStats runs[SIM_RUNS]);

void print_overall_summary(RunStats results[ALG_COUNT][SIM_RUNS]);

#endif
