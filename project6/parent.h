/* parent.h - Parent process select() loop + output.txt writer
 * COEN 383 Project 6
 *
 * Person 4: parent select() loop + output.txt
 */
#ifndef PARENT_H
#define PARENT_H

#include "utils.h"

/* Run the parent's multiplexed read loop.
 *   read_fds[]: the 5 read ends, one per child
 * On return, all 5 children have closed their write ends.
 */
void run_parent(int read_fds[NUM_CHILDREN]);

#endif /* PARENT_H */
