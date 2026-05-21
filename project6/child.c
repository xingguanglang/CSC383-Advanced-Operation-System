/* child.c - implementations for the four worker children and child 5
 * COEN 383 Project 6
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/select.h>

#include "child.h"
#include "utils.h"

/* ================================================================== */
/*   PERSON 2:  Child 1-4 automatic message logic                     */
/* ================================================================== */

void run_worker_child(int child_id, int write_fd)
{
    /* Seed RNG with high-resolution time + pid + child_id so all five
     * children, forked in the same microsecond, get distinct streams.
     */
    struct timeval seed_tv;
    gettimeofday(&seed_tv, NULL);
    srand((unsigned)((unsigned)getpid() * 2654435761u
                     ^ (unsigned)seed_tv.tv_usec
                     ^ ((unsigned)child_id << 16)));

    int  msg_no = 1;
    char ts[TS_SIZE];
    char msg[LINE_SIZE];

    while (elapsed_seconds() < (double)DURATION_SEC) {

        /* sleep 0, 1, or 2 seconds (per assignment) */
        int s = rand() % 3;
        if (s > 0) sleep(s);

        if (elapsed_seconds() >= (double)DURATION_SEC) break;

        format_timestamp(ts, sizeof(ts));
        int n = snprintf(msg, sizeof(msg),
                         "%s: Child %d message %d\n",
                         ts, child_id, msg_no++);
        if (n > 0) {
            if (write(write_fd, msg, (size_t)n) < 0) {
                /* parent may have closed early; just stop */
                break;
            }
        }
    }

    /* Closing the write end gives the parent EOF on its read end --
     * per the preview-PDF's P.S. note, that is how the parent learns
     * this child has exited.
     */
    close(write_fd);
    exit(0);
}

/* ================================================================== */
/*   PERSON 3:  Child 5 terminal interaction logic                    */
/* ================================================================== */

void run_console_child(int child_id, int write_fd)
{
    char ts[TS_SIZE];
    char chunk[LINE_SIZE];
    char line[LINE_SIZE];
    char msg[LINE_SIZE + 64];
    size_t linelen = 0;
    int    msg_no  = 1;

    fd_set rset;
    struct timeval tv;

    /* Initial prompt. Assignment says "prompt at the terminal (standard
     * out)" -- so we print to stdout, not stderr.
     */
    fprintf(stdout, "Child %d prompt> ", child_id);
    fflush(stdout);

    while (elapsed_seconds() < (double)DURATION_SEC) {

        double remaining = (double)DURATION_SEC - elapsed_seconds();
        if (remaining <= 0) break;

        /* Use select() on stdin alone, with the remaining lifetime as
         * timeout. That way the child still exits exactly at 30 s even
         * if the user never types anything.
         */
        FD_ZERO(&rset);
        FD_SET(STDIN_FILENO, &rset);
        tv.tv_sec  = (time_t)remaining;
        tv.tv_usec = (suseconds_t)((remaining - (time_t)remaining) * 1000000);

        int r = select(STDIN_FILENO + 1, &rset, NULL, NULL, &tv);
        if (r < 0) {
            if (errno == EINTR) continue;
            break;
        }
        if (r == 0) break;                           /* timed out -> 30 s */
        if (!FD_ISSET(STDIN_FILENO, &rset)) continue;

        ssize_t n = read(STDIN_FILENO, chunk, sizeof(chunk));
        if (n <= 0) break;                           /* EOF / error      */

        /* When stdin is itself a pipe (e.g. ./project6 < input.txt) a
         * single read() can return several lines.  Split on '\n' so each
         * user line becomes its own pipe message.
         */
        ssize_t k;
        for (k = 0; k < n; k++) {
            char c = chunk[k];
            if (c == '\n' || c == '\r') {
                if (linelen > 0) {
                    line[linelen] = '\0';

                    format_timestamp(ts, sizeof(ts));
                    int m = snprintf(msg, sizeof(msg),
                                     "%s: Child %d: msg %d from terminal: %s\n",
                                     ts, child_id, msg_no++, line);
                    if (m > 0) {
                        if (write(write_fd, msg, (size_t)m) < 0) {
                            close(write_fd);
                            exit(0);
                        }
                    }
                    linelen = 0;

                    /* immediately prompt for the next message */
                    fprintf(stdout, "Child %d prompt> ", child_id);
                    fflush(stdout);
                }
            } else if (linelen < LINE_SIZE - 1) {
                line[linelen++] = c;
            }
        }
    }

    /* Flush any tail without trailing newline */
    if (linelen > 0) {
        line[linelen] = '\0';
        format_timestamp(ts, sizeof(ts));
        int m = snprintf(msg, sizeof(msg),
                         "%s: Child %d: msg %d from terminal: %s\n",
                         ts, child_id, msg_no, line);
        if (m > 0) (void)!write(write_fd, msg, (size_t)m);
    }

    close(write_fd);
    exit(0);
}
