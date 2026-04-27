/* ============================================================
 * main.c - 主程序入口
 * 负责人: Member 1 (Team Lead)
 *
 * 流程:
 *   1. 生成一份固定的工作负载 (所有算法共用同一组 jobs, 公平比较)
 *   2. 对 5 种算法各跑 5 次, 收集统计
 *   3. 仅对 FIFO 第 1 次运行打印详细 trace (前 100 个引用 + ENTER/EXIT 事件)
 *      —— 避免日志爆炸,但仍满足 "Run the simulator for 100 page references" 要求
 *   4. 输出每种算法 5 次平均的 hit/miss 比与平均换入进程数
 *
 * 编译: 见 Makefile
 * ============================================================ */
#include "common.h"
#include "workload.h"
#include "page_table.h"
#include "simulator.h"
#include "stats.h"

int main(int argc, char *argv[]) {
    unsigned int seed = (argc > 1) ? (unsigned)atoi(argv[1])
                                   : (unsigned)time(NULL);
    srand(seed);
    printf("Random seed = %u\n", seed);

    /* === 1. 生成基础工作负载 === */
    Process *workload = generate_workload(NUM_PROCESSES);
    print_workload(workload);

    /* === 2. 主实验: 每个算法 5 次 === */
    RunStats results[ALG_COUNT][SIM_RUNS];

    for (int a = 0; a < ALG_COUNT; a++) {
        printf("\n>>>>>>>>>>>>>>>>>> Algorithm: %s <<<<<<<<<<<<<<<<<<\n",
               ALG_NAMES[a]);
        for (int r = 0; r < SIM_RUNS; r++) {
            /* 第一种算法的第一次运行: verbose=true,
             * 用于满足"打印 100 次引用"和"ENTER/EXIT 事件"的要求 */
            bool verbose = (a == ALG_FIFO && r == 0);
            results[a][r] = run_simulation((AlgorithmType)a, workload, verbose);
            print_run_stats((AlgorithmType)a, r, results[a][r]);
        }
        print_alg_summary((AlgorithmType)a, results[a]);
    }

    /* === 3. 总结对比 === */
    print_overall_summary(results);

    free_workload(workload);
    return 0;
}
