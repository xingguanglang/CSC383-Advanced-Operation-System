#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "common.h"

RunStats run_simulation(AlgorithmType alg,
                        Process *workload_template,
                        bool verbose);

int next_page_with_locality(int i, int size);

Process *clone_workload(Process *src);

#endif
