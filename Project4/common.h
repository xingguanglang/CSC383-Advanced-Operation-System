#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>

#define TOTAL_FRAMES        100      
#define NUM_PROCESSES       150      
#define MIN_FREE_TO_RUN     4       
#define SIM_DURATION_SEC    60       
#define SIM_RUNS            5        
#define REF_INTERVAL_MS     100      
#define PRINT_REF_LIMIT     100      
#define MAX_PROC_NAME       8

static const int PROC_SIZES[]    = {5, 11, 17, 31};
static const int PROC_DURATIONS[] = {1, 2, 3, 4, 5};

typedef enum {
    ALG_FIFO = 0,
    ALG_LRU,
    ALG_LFU,
    ALG_MFU,
    ALG_RANDOM,
    ALG_COUNT
} AlgorithmType;

static const char *ALG_NAMES[ALG_COUNT] = {
    "FIFO", "LRU", "LFU", "MFU", "RANDOM"
};

typedef struct Process {
    char  name[MAX_PROC_NAME];   
    int   pid;                   
    int   size_pages;            
    int   arrival_ms;            
    int   duration_ms;           
    int   start_ms;              
    int   finish_ms;             
    int   current_page;          
    bool  is_running;            
    bool  is_done;               
    int   pages_resident;        
    struct Process *next;
} Process;

typedef struct Frame {
    bool   in_use;               
    int    owner_pid;            
    char   owner_name[MAX_PROC_NAME];
    int    page_num;             
    int    load_time_ms;         
    int    last_used_ms;         
    int    use_count;            
} Frame;

typedef struct {
    long hits;
    long misses;
    int  processes_swapped_in;   
    int  processes_completed;   
} RunStats;

#endif
