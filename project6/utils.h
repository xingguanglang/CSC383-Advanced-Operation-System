/* utils.h - Shared constants, types, and helpers
 * COEN 383 Project 6
 */
#ifndef UTILS_H
#define UTILS_H

#include <sys/time.h>
#include <stddef.h>

/* Pipe end indices (matches instructor's pipe-1 example) */
#define READ_END        0
#define WRITE_END       1

/* Project-wide parameters */
#define NUM_CHILDREN    5
#define DURATION_SEC    30
#define BUF_SIZE        512
#define LINE_SIZE       256
#define TS_SIZE         32

/* Reference start time of the whole program. Set once by the parent
 * before fork(), so every child inherits the same baseline.
 */
extern struct timeval g_start;

/* Return seconds elapsed since g_start (as a double). */
double elapsed_seconds(void);

/* Format the elapsed time into "S:SS.mmm" (seconds-since-start, ms).
 * Buffer must be at least TS_SIZE bytes.
 */
void   format_timestamp(char *out, size_t outsz);

#endif /* UTILS_H */
