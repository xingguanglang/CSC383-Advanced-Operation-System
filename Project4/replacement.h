#ifndef REPLACEMENT_H
#define REPLACEMENT_H

#include "common.h"

int select_victim_fifo(int now_ms);
int select_victim_lru(int now_ms);
int select_victim_lfu(int now_ms);
int select_victim_mfu(int now_ms);
int select_victim_random(int now_ms);

int select_victim(AlgorithmType alg, int now_ms);

#endif
