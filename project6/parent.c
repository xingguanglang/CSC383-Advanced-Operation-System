/* parent.c - parent process: select() over 5 pipes, write to output.txt
 * COEN 383 Project 6
 *
 * Follows the instructor's select-1 example: FD_ZERO / FD_SET / FD_ISSET,
 * switch(result) with three cases (timeout=0, error=-1, default=ready),
 * and ioctl(FIONREAD) to learn how many bytes are queued before read().
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <sys/wait.h>

#include "parent.h"
#include "utils.h"

/* Per-pipe line-assembly buffer.  A single read() from a pipe is not
 * guaranteed to be line-aligned, so each pipe accumulates bytes here
 * and flushes one tagged line whenever a '\n' is seen.
 */
typedef struct {
    char   buf[BUF_SIZE];
    size_t len;
    int    alive;
} pipe_state_t;

/* Emit one fully-assembled child line to output.txt with the parent's
 * own timestamp prepended.  Result line has TWO timestamps:
 *
 *   [parent S:SS.mmm] S:SS.mmm: Child K message N
 *      ^parent_ts       ^child_ts (already in the child's text)
 */
static void emit_line(FILE *out, const char *child_line)
{
    char ts[TS_SIZE];
    format_timestamp(ts, sizeof(ts));
    fprintf(out, "[parent %s] %s\n", ts, child_line);
    fflush(out);
}

/* Push raw bytes into pipe i's line buffer; flush whenever '\n' is hit. */
static void absorb_bytes(pipe_state_t *ps, const char *data, ssize_t n,
                         FILE *out)
{
    ssize_t k;
    for (k = 0; k < n; k++) {
        char c = data[k];
        if (c == '\n') {
            ps->buf[ps->len] = '\0';
            if (ps->len > 0) emit_line(out, ps->buf);
            ps->len = 0;
        } else if (ps->len < BUF_SIZE - 1) {
            ps->buf[ps->len++] = c;
        } else {
            /* line longer than BUF_SIZE -> force flush */
            ps->buf[ps->len] = '\0';
            emit_line(out, ps->buf);
            ps->len = 0;
            ps->buf[ps->len++] = c;
        }
    }
}

void run_parent(int read_fds[NUM_CHILDREN])
{
    FILE *out = fopen("output.txt", "w");
    if (!out) {
        perror("fopen(output.txt)");
        exit(1);
    }

    pipe_state_t ps[NUM_CHILDREN];
    int i;
    int alive_count = NUM_CHILDREN;
    for (i = 0; i < NUM_CHILDREN; i++) {
        ps[i].len   = 0;
        ps[i].alive = 1;
        ps[i].buf[0] = '\0';
    }

    /* "inputs" holds the canonical set (instructor's select-1 style);
     * "inputfds" is the working copy passed to select() each loop, because
     * select() modifies its fd_set arguments in place.
     */
    fd_set inputs, inputfds;

    while (alive_count > 0) {

        /* Rebuild the canonical fd_set from currently-alive pipes. */
        FD_ZERO(&inputs);
        int maxfd = -1;
        for (i = 0; i < NUM_CHILDREN; i++) {
            if (ps[i].alive) {
                FD_SET(read_fds[i], &inputs);
                if (read_fds[i] > maxfd) maxfd = read_fds[i];
            }
        }

        inputfds = inputs;

        /* No timeout: block until one of the pipes has data or EOF.
         * (The instructor's standalone select-1 demo used a 2 s timeout
         * just to demonstrate the timeout case; here the parent has
         * nothing else to do, so blocking forever is the right call.)
         */
        int result = select(maxfd + 1, &inputfds, NULL, NULL, NULL);

        switch (result) {

        case -1:                                /* error */
            if (errno == EINTR) continue;
            perror("select");
            goto done;

        case 0:                                 /* timeout (won't happen) */
            continue;

        default:                                /* one or more fds ready */
            for (i = 0; i < NUM_CHILDREN; i++) {
                if (!ps[i].alive) continue;
                if (!FD_ISSET(read_fds[i], &inputfds)) continue;

                /* Ask the kernel how many bytes are available right now
                 * (style borrowed from instructor's select-1 example).
                 */
                int nready = 0;
                if (ioctl(read_fds[i], FIONREAD, &nready) < 0) nready = 0;

                char tmp[BUF_SIZE];
                size_t want = (nready > 0 && (size_t)nready < sizeof(tmp))
                                  ? (size_t)nready : sizeof(tmp);
                ssize_t n = read(read_fds[i], tmp, want);

                if (n < 0) {
                    if (errno == EINTR) continue;
                    perror("read");
                    close(read_fds[i]);
                    ps[i].alive = 0;
                    alive_count--;
                    continue;
                }

                if (n == 0) {
                    /* EOF -> per assignment P.S., the child closed its
                     * write end on exit. Flush any partial trailing line.
                     */
                    if (ps[i].len > 0) {
                        ps[i].buf[ps[i].len] = '\0';
                        emit_line(out, ps[i].buf);
                        ps[i].len = 0;
                    }
                    close(read_fds[i]);
                    ps[i].alive = 0;
                    alive_count--;
                    continue;
                }

                absorb_bytes(&ps[i], tmp, n, out);
            }
            break;
        }
    }

done:
    fclose(out);

    /* Reap zombies (Person 5: required to avoid zombie processes). */
    int status;
    while (wait(&status) > 0) { /* loop until no more children */ }
}
