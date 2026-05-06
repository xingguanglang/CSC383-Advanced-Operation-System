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

    Process *workload = generate_workload(NUM_PROCESSES);
    print_workload(workload);

    RunStats results[ALG_COUNT][SIM_RUNS];

    for (int a = 0; a < ALG_COUNT; a++) {
        printf("\n>>>>>>>>>>>>>>>>>> Algorithm: %s <<<<<<<<<<<<<<<<<<\n",
               ALG_NAMES[a]);
        for (int r = 0; r < SIM_RUNS; r++) {
            bool verbose = (a == ALG_FIFO && r == 0);
            results[a][r] = run_simulation((AlgorithmType)a, workload, verbose);
            print_run_stats((AlgorithmType)a, r, results[a][r]);
        }
        print_alg_summary((AlgorithmType)a, results[a]);
    }

    print_overall_summary(results);

    free_workload(workload);
    return 0;
}
