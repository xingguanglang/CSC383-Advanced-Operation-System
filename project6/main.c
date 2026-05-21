/* main.c - Program entry point
 * COEN 383 Project 6 - UNIX I/O with pipes, fork(), and select()
 *
 * Person 1: project structure + pipe/fork setup
 *
 * Architecture (from preview PDF):
 *
 *                       Parent
 *                      [select()]
 *                /    /    |    \    \
 *              C1   C2    C3    C4    C5
 *                                      |
 *                                   Console
 *
 *   pipes[0..4]    one pipe per child
 *   parent reads,  children write
 *   children 1-4   automatic timestamped messages
 *   child   5      reads from terminal stdin
 *   output.txt     written by parent, two timestamps per line
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>

#include "utils.h"
#include "child.h"
#include "parent.h"

int main(void)
{
    int pipes[NUM_CHILDREN][2];
    int i, j;

    /* Anchor the program's time origin BEFORE forking so every child
     * inherits the same baseline (their first message reads "0:00.xxx").
     */
    gettimeofday(&g_start, NULL);

    /* ---------- 1. create 5 pipes ----------------------------------- */
    for (i = 0; i < NUM_CHILDREN; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            exit(1);
        }
    }

    /* ---------- 2. fork 5 children ---------------------------------- */
    for (i = 0; i < NUM_CHILDREN; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            exit(1);
        }

        if (pid == 0) {
            /* ----- CHILD i (0-indexed); child_id is 1..5 ------------ */
            int child_id = i + 1;

            /* Close every pipe end this child does NOT need:
             *   - all read  ends (children never read from any pipe)
             *   - all write ends except this child's own one
             * Forgetting any of these means the parent will never see
             * EOF after this child exits.
             */
            for (j = 0; j < NUM_CHILDREN; j++) {
                close(pipes[j][READ_END]);
                if (j != i) close(pipes[j][WRITE_END]);
            }

            int my_write = pipes[i][WRITE_END];

            if (child_id <= 4) {
                run_worker_child(child_id, my_write);   /* Person 2 */
            } else {
                run_console_child(child_id, my_write);  /* Person 3 */
            }
            /* run_*_child() never returns -- they call exit(0). */
            _exit(0);
        }
        /* parent keeps forking */
    }

    /* ---------- 3. parent: close all write ends, then select() ------ */
    int read_fds[NUM_CHILDREN];
    for (i = 0; i < NUM_CHILDREN; i++) {
        close(pipes[i][WRITE_END]);
        read_fds[i] = pipes[i][READ_END];
    }

    run_parent(read_fds);                               /* Person 4 */

    printf("All children terminated. Output written to output.txt\n");
    return 0;
}
