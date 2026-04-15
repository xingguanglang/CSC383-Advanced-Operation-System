/*
 * process.h - Data structures, constants, and shared declarations
 * COEN 383 - Project 2: Process Scheduling Algorithms
 */

#ifndef PROCESS_H
#define PROCESS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <float.h>

/* ─── Constants ─── */

#define MAX_QUANTA       100
#define MAX_PROCESSES    200
#define NUM_RUNS         5
#define NUM_PRIORITIES   4
#define AGING_THRESHOLD  5
#define RR_TIME_SLICE    1
#define TIMELINE_SIZE    500

/* ─── Data Structures ─── */

typedef struct {
    char   name;               /* A-Z, a-z */
    int    arrival_time;
    int    service_time;       /* expected total run time */
    int    priority;           /* 1 (highest) .. 4 */
    int    remaining_time;
    int    start_time;         /* first time scheduled (-1 = not started) */
    int    finish_time;
    int    started;
    int    finished;
    int    waited_at_level;    /* for aging */
    int    original_priority;
    int    effective_priority;
} Process;

typedef struct {
    double avg_turnaround;
    double avg_waiting;
    double avg_response;
    double throughput;
} Stats;

typedef struct {
    Stats  per_priority[NUM_PRIORITIES + 1]; /* index 1..4 */
    Stats  overall;
} HPFStats;

/* ─── Global Variables (defined in process.c) ─── */

extern Process processes[MAX_PROCESSES];
extern int     num_processes;
extern char    timeline[TIMELINE_SIZE];
extern int     timeline_len;

/* ─── process.c functions ─── */

void generate_processes(unsigned int seed);
void reset_processes(void);
void print_processes(void);
int  all_started_done(void);

/* ─── stats.c functions ─── */

Stats    compute_stats(int end_time);
HPFStats compute_hpf_stats(int end_time);
void     print_timeline(int len);
void     print_stats(Stats *s);
void     print_hpf_stats(HPFStats *hs);

/* ─── Scheduling algorithm functions ─── */

int run_fcfs(void);          /* fcfs.c   */
int run_sjf(void);           /* sjf.c    */
int run_srt(void);           /* srt.c    */
int run_rr(void);            /* rr.c     */
int run_hpf_np(int aging);   /* hpf_np.c */
int run_hpf_p(int aging);    /* hpf_p.c  */

#endif /* PROCESS_H */
