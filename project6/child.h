/* child.h - Child process behaviors
 * COEN 383 Project 6
 *
 * Person 2: Children 1-4  -> run_worker_child()
 * Person 3: Child 5       -> run_console_child()
 */
#ifndef CHILD_H
#define CHILD_H

/* Children 1-4: random sleep (0/1/2 s) then send a timestamped message
 * to its pipe. Loops until DURATION_SEC, then closes write_fd and exits.
 */
void run_worker_child(int child_id, int write_fd);

/* Child 5: prompts on stdout, reads a line from stdin, writes a
 * timestamped message to its pipe. Honors the 30 s lifetime even if
 * the user never types anything.
 */
void run_console_child(int child_id, int write_fd);

#endif /* CHILD_H */
