# COEN 383 Project-6 — UNIX I/O with pipes, fork(), and select()

## Files

```
main.c          Person 1   program entry, pipe + fork setup
child.h/.c      Person 2   Children 1-4 automatic timestamped messages
                Person 3   Child 5 terminal interaction
parent.h/.c     Person 4   select() multiplex loop + output.txt
utils.h/.c      shared     timestamp helpers, READ_END/WRITE_END constants
Makefile        Person 5   build / test / package
sample_input.txt           sample stdin used for the included output.txt
output.txt                 sample output from a 30 s run
README.md                  this file
```

## Build & Run

```
make                       # build ./project6
./project6                 # run interactively (type lines for Child 5)
make sample                # run with sample_input.txt fed as stdin
make clean                 # remove binaries and output.txt
make zip GROUP=Group-3     # produce Group-3.zip for Camino
```

## Architecture

```
                       Parent
                      [select()]
                /    /    |    \    \
              pipe1 pipe2 pipe3 pipe4 pipe5
                |    |    |    |    |
              C1   C2   C3   C4   C5
                                    |
                                 Console (stdin/stdout)
                                    
              Parent ─────────────────> output.txt
```

- Parent creates 5 pipes, forks 5 children, then closes every write end
  it inherited. Each child closes every pipe end except its own write end.
  This guarantees that closing the write end on child exit propagates as
  EOF to the parent's read end.
- **Children 1–4** loop: `sleep(rand()%3)` → `gettimeofday()` →
  `write("S:SS.mmm: Child K message N\n")` → repeat for 30 s.
- **Child 5** loops: prompt on stdout → `select(stdin, remaining_time)` →
  read line → write `"S:SS.mmm: Child 5: msg N from terminal: <text>\n"`
  to its pipe → re-prompt. Exits at 30 s even if no input arrives.
- **Parent** uses `select()` with `FD_ZERO/FD_SET/FD_ISSET` exactly in the
  style of the instructor's `select-1` example, including a `switch` on
  the return value and `ioctl(FIONREAD)` to size each read.

## Output format

Each line in `output.txt` carries two timestamps:

```
[parent S:SS.mmm] S:SS.mmm: Child K message N
       ^parent ts        ^child ts
```

- First timestamp is added by the parent when it reads the message.
- Second timestamp was generated earlier by the child.
- The gap between them is pipe + scheduler latency, usually < 5 ms.

## Sample I/O

`sample_input.txt`:

```
first user input
second message
third line typed
fourth input
fifth and final
```

First lines of the produced `output.txt`:

```
[parent 0:00.004] 0:00.003: Child 2 message 1
[parent 0:00.004] 0:00.003: Child 4 message 1
[parent 0:00.004] 0:00.003: Child 5: msg 1 from terminal: first user input
[parent 0:00.004] 0:00.003: Child 5: msg 2 from terminal: second message
...
```

## Brief report: issues encountered

1. **Pipe-end housekeeping.** Each pipe's write end must exist in exactly
   one process (the matching child); otherwise the parent never receives
   EOF on that pipe and `select()` will block forever. We close all read
   ends in every child and close all write ends in the parent after
   forking, plus every "other child's" write end inside each child.

2. **Per-pipe line assembly.** A single `read()` from a pipe is not
   line-aligned — it can return a partial line, a full line, or several
   lines glued together. Each pipe keeps its own line buffer in the
   parent and only flushes a tagged output line when it sees `\n`.

3. **Child 5 multi-line input.** When stdin is itself a pipe (e.g.
   `./project6 < sample_input.txt`), `read()` can return several lines
   at once. Child 5 splits on `\n` so each user line becomes its own
   timestamped pipe message instead of being merged.

4. **Random-seed collisions.** All five children are forked within a
   few microseconds, so `srand(getpid() ^ time(NULL))` produced almost
   identical streams and one child dominated the output. We now mix
   `tv_usec`, a multiplicative hash of `pid`, and the child id into the
   seed, which gives each child an independent stream.

5. **30-second deadline for Child 5.** Child 5 must not block on stdin
   forever, so it calls `select()` on stdin with the *remaining*
   lifetime as the timeout. If the user never types, it still exits
   exactly at 30 s.

6. **EOF as the "child exited" signal.** The preview PDF's P.S. note
   spells this out: when the child closes its write end on exit, the
   parent's `select()` flags the pipe readable and the next `read()`
   returns 0. We rely on this rather than `SIGCHLD` so the parent's
   I/O loop stays simple.

7. **Zombie reaping.** After the parent leaves the select loop it
   calls `wait()` in a loop to clean up every child, avoiding zombies.

## Division of labor

```
Person 1 — main.c                    program skeleton, pipe + fork setup
Person 2 — child.c::run_worker_child  Children 1-4 (auto messages)
Person 3 — child.c::run_console_child Child 5 (terminal interaction)
Person 4 — parent.c                   select() loop, output.txt writing
Person 5 — Makefile, README, sample   testing, packaging, this report
```
