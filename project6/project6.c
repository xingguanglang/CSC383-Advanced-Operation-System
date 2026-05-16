/*
 * COEN 383 - Project 6: UNIX I/O with pipes and select()
 *
 * Parent forks 5 children, each connected via a dedicated pipe.
 *   - Children 1-4: sleep random(0,1,2)s, write timestamped message to pipe
 *   - Child 5: prompt user at terminal, read line, write timestamped msg to pipe
 *   - All children run for 30 seconds, then close write-end and exit
 *   - Parent uses select() to multiplex reads from all 5 pipes,
 *     prepends its own timestamp, and writes to output.txt
 *   - Parent exits after all 5 children have closed their pipes (EOF)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <time.h>

#define NUM_CHILDREN   5
#define RUN_SECONDS    30
#define READ_END       0
#define WRITE_END      1
#define BUF_SIZE       512
#define LINE_SIZE      256

/* Global reference start time, set by parent before forking, inherited by children */
static struct timeval g_start;

/* ------------------------------------------------------------------ */
/* Helper: return seconds elapsed since g_start (as double).          */
/* ------------------------------------------------------------------ */
static double elapsed_seconds(void)
{
    struct timeval now;
    gettimeofday(&now, NULL);
    double sec  = (double)(now.tv_sec  - g_start.tv_sec);
    double usec = (double)(now.tv_usec - g_start.tv_usec);
    return sec + usec / 1000000.0;
}

/* ------------------------------------------------------------------ */
/* Helper: format the elapsed time into "s:mmm.uuu" style "S:MM.mmm". */
/*   e.g.  0:00.123,  0:02.456, 12:34.789                             */
/* ------------------------------------------------------------------ */
static void format_timestamp(char *out, size_t outsz)
{
    double e = elapsed_seconds();
    int    whole_sec = (int)e;
    int    msec      = (int)((e - whole_sec) * 1000.0);
    if (msec >= 1000) { msec = 999; }
    /* Format "S:SS.mmm" -- minutes:seconds.milliseconds */
    snprintf(out, outsz, "%d:%02d.%03d", whole_sec / 60, whole_sec % 60, msec);
}

/* ------------------------------------------------------------------ */
/* Child 1-4 behavior: random sleep, send timestamped message.        */
/* ------------------------------------------------------------------ */
static void run_worker_child(int child_id, int write_fd)
{
    /* Re-seed RNG per child with high-resolution + pid so streams differ */
    struct timeval seed_tv;
    gettimeofday(&seed_tv, NULL);
    srand((unsigned)(getpid() * 2654435761u
                     ^ (unsigned)seed_tv.tv_usec
                     ^ ((unsigned)child_id << 16)));

    int msg_no = 1;
    char ts[32];
    char msg[LINE_SIZE];

    while (elapsed_seconds() < (double)RUN_SECONDS) {
        int s = rand() % 3;          /* 0, 1, or 2 seconds */
        if (s > 0) sleep(s);

        if (elapsed_seconds() >= (double)RUN_SECONDS) break;

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

    close(write_fd);
    exit(0);
}

/* ------------------------------------------------------------------ */
/* Child 5 behavior: prompt user, read line from stdin, write to pipe */
/* ------------------------------------------------------------------ */
static void run_console_child(int child_id, int write_fd)
{
    char ts[32];
    char chunk[LINE_SIZE];
    char line[LINE_SIZE];
    char msg[LINE_SIZE + 64];
    size_t linelen = 0;
    int  msg_no = 1;

    fd_set rset;
    struct timeval tv;

    /* Print initial prompt once; subsequent prompts after each line */
    fprintf(stderr, "Child 5 prompt> ");
    fflush(stderr);

    while (elapsed_seconds() < (double)RUN_SECONDS) {
        double remaining = (double)RUN_SECONDS - elapsed_seconds();
        if (remaining <= 0) break;

        FD_ZERO(&rset);
        FD_SET(STDIN_FILENO, &rset);
        tv.tv_sec  = (time_t)remaining;
        tv.tv_usec = (suseconds_t)((remaining - (time_t)remaining) * 1000000);

        int r = select(STDIN_FILENO + 1, &rset, NULL, NULL, &tv);
        if (r < 0) {
            if (errno == EINTR) continue;
            break;
        }
        if (r == 0) break;           /* 30s expired */
        if (!FD_ISSET(STDIN_FILENO, &rset)) continue;

        ssize_t n = read(STDIN_FILENO, chunk, sizeof(chunk));
        if (n <= 0) break;           /* EOF or error */

        /* Walk through bytes, emit one pipe message per newline-terminated line.
         * This is needed because when stdin is a pipe (non-terminal) we may
         * receive multiple lines in a single read().
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
                    /* re-prompt for next line */
                    fprintf(stderr, "Child 5 prompt> ");
                    fflush(stderr);
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

/* ------------------------------------------------------------------ */
/* Parent: select() loop reading from all child pipes; write output.  */
/* ------------------------------------------------------------------ */
static void run_parent(int read_fds[NUM_CHILDREN])
{
    FILE *out = fopen("output.txt", "w");
    if (!out) {
        perror("fopen(output.txt)");
        exit(1);
    }

    /* Per-pipe line-assembly buffers, since reads may not align to lines */
    char    linebuf[NUM_CHILDREN][BUF_SIZE];
    size_t  linelen[NUM_CHILDREN];
    int     alive  [NUM_CHILDREN];
    int     i;

    for (i = 0; i < NUM_CHILDREN; i++) {
        linelen[i] = 0;
        alive[i]   = 1;
        linebuf[i][0] = '\0';
    }

    int alive_count = NUM_CHILDREN;

    while (alive_count > 0) {
        fd_set rset;
        FD_ZERO(&rset);
        int maxfd = -1;
        for (i = 0; i < NUM_CHILDREN; i++) {
            if (alive[i]) {
                FD_SET(read_fds[i], &rset);
                if (read_fds[i] > maxfd) maxfd = read_fds[i];
            }
        }

        /* No timeout: block until at least one fd is ready, or all done */
        int r = select(maxfd + 1, &rset, NULL, NULL, NULL);
        if (r < 0) {
            if (errno == EINTR) continue;
            perror("select");
            break;
        }

        for (i = 0; i < NUM_CHILDREN; i++) {
            if (!alive[i]) continue;
            if (!FD_ISSET(read_fds[i], &rset)) continue;

            char tmp[BUF_SIZE];
            ssize_t n = read(read_fds[i], tmp, sizeof(tmp));
            if (n < 0) {
                if (errno == EINTR) continue;
                perror("read");
                close(read_fds[i]);
                alive[i] = 0;
                alive_count--;
                continue;
            }
            if (n == 0) {
                /* EOF: child closed its write end -> child exited */
                /* Flush any partially buffered tail */
                if (linelen[i] > 0) {
                    linebuf[i][linelen[i]] = '\0';
                    char ts[32];
                    format_timestamp(ts, sizeof(ts));
                    fprintf(out, "[parent %s] %s\n", ts, linebuf[i]);
                    linelen[i] = 0;
                }
                close(read_fds[i]);
                alive[i] = 0;
                alive_count--;
                continue;
            }

            /* Feed bytes into the per-pipe line buffer; flush on '\n' */
            ssize_t k;
            for (k = 0; k < n; k++) {
                char c = tmp[k];
                if (c == '\n') {
                    linebuf[i][linelen[i]] = '\0';
                    if (linelen[i] > 0) {
                        char ts[32];
                        format_timestamp(ts, sizeof(ts));
                        /* Each output line carries TWO timestamps:
                         *   [parent_ts]  child_ts: child message text
                         */
                        fprintf(out, "[parent %s] %s\n", ts, linebuf[i]);
                        fflush(out);
                    }
                    linelen[i] = 0;
                } else if (linelen[i] < BUF_SIZE - 1) {
                    linebuf[i][linelen[i]++] = c;
                } else {
                    /* line too long: force flush */
                    linebuf[i][linelen[i]] = '\0';
                    char ts[32];
                    format_timestamp(ts, sizeof(ts));
                    fprintf(out, "[parent %s] %s\n", ts, linebuf[i]);
                    linelen[i] = 0;
                    linebuf[i][linelen[i]++] = c;
                }
            }
        }
    }

    fclose(out);

    /* Reap all children */
    int status;
    while (wait(&status) > 0) { /* loop */ }
}

/* ------------------------------------------------------------------ */
/* main                                                               */
/* ------------------------------------------------------------------ */
int main(void)
{
    int pipes[NUM_CHILDREN][2];
    int i, j;

    gettimeofday(&g_start, NULL);

    /* Create 5 pipes */
    for (i = 0; i < NUM_CHILDREN; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            exit(1);
        }
    }

    /* Fork 5 children */
    for (i = 0; i < NUM_CHILDREN; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            exit(1);
        }
        if (pid == 0) {
            /* CHILD i (0-indexed) -- child_id = i+1 */
            int child_id = i + 1;

            /* Close all pipes except our own write end */
            for (j = 0; j < NUM_CHILDREN; j++) {
                close(pipes[j][READ_END]);            /* never reads from any pipe */
                if (j != i) close(pipes[j][WRITE_END]); /* not our pipe */
            }

            int my_write = pipes[i][WRITE_END];

            if (child_id <= 4) {
                run_worker_child(child_id, my_write);
            } else {
                run_console_child(child_id, my_write);
            }
            /* never returns */
            _exit(0);
        }
        /* parent continues forking */
    }

    /* PARENT: close every write end (parent never writes to pipes) */
    int read_fds[NUM_CHILDREN];
    for (i = 0; i < NUM_CHILDREN; i++) {
        close(pipes[i][WRITE_END]);
        read_fds[i] = pipes[i][READ_END];
    }

    run_parent(read_fds);

    printf("All children terminated. Output written to output.txt\n");
    return 0;
}
