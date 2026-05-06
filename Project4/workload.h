#ifndef WORKLOAD_H
#define WORKLOAD_H

#include "common.h"

Process *generate_workload(int count);

void free_workload(Process *head);

void print_workload(Process *head);

#endif
