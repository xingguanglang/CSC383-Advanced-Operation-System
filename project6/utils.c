/* utils.c - timestamp helpers shared by parent and children
 * COEN 383 Project 6
 */
#include <stdio.h>
#include <sys/time.h>
#include "utils.h"

struct timeval g_start;

double elapsed_seconds(void)
{
    struct timeval now;
    gettimeofday(&now, NULL);
    double sec  = (double)(now.tv_sec  - g_start.tv_sec);
    double usec = (double)(now.tv_usec - g_start.tv_usec);
    return sec + usec / 1000000.0;
}

/* Per the preview PDF:
 *   int sec     = (int)(tv.tv_sec);
 *   double msec = ((double)tv.tv_usec) / 1000;
 * We produce "S:SS.mmm" using elapsed-since-start, so the very first
 * message reads "0:00.xxx" as the assignment example shows.
 */
void format_timestamp(char *out, size_t outsz)
{
    double e = elapsed_seconds();
    int    whole_sec = (int)e;
    int    msec      = (int)((e - whole_sec) * 1000.0);
    if (msec >= 1000) msec = 999;
    snprintf(out, outsz, "%d:%02d.%03d",
             whole_sec / 60, whole_sec % 60, msec);
}
