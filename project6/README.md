# COEN 383 Project-6 — UNIX I/O with pipes and select()

## Files
- `project6.c`     — main source
- `Makefile`       — `make` / `make run` / `make sample` / `make clean`
- `sample_input.txt` — canned stdin used for the included `output.txt`
- `output.txt`     — sample output from a 30-second run
- `README.md`      — this report

## Build & Run
```
make                       # build ./project6
./project6                 # run interactively (you type for child 5)
make sample                # run with sample_input.txt as stdin
```

## Architecture
```
                Parent  -- select() over 5 read-ends --> output.txt
                 ^  ^  ^  ^  ^
                 |  |  |  |  |     (each pipe write-end held by exactly one child)
                pipe1..5
                 |  |  |  |  |
                 C1 C2 C3 C4 C5 <-- stdin (terminal)
```
- **Parent** creates 5 pipes, forks 5 children. Each child keeps the write end
  of exactly one pipe; everything else is closed in both parent and child.
  Parent uses `select()` (blocking, no timeout) to wait on the 5 read ends.
- **Children 1–4** loop: `sleep(rand()%3)` → format timestamp via
  `gettimeofday()` → write `"S:SS.mmm: Child K message N\n"` to its pipe.
- **Child 5** loops: prompts on stderr, uses `select()` on stdin with the
  remaining time as the timeout (so it honors the 30 s deadline even if the
  user never types), then reads input and writes
  `"S:SS.mmm: Child 5: msg N from terminal: <text>\n"` to its pipe.
- All children exit when 30 s elapse; closing the write end gives the parent
  EOF (`read() == 0`) on that pipe, at which point the parent removes the fd
  from the active set. When all 5 are gone, parent reaps with `wait()` and exits.

## Output format
Each output line has two timestamps:
```
[parent S:SS.mmm] S:SS.mmm: Child K message N
```
- First timestamp = when the *parent* read the line.
- Second timestamp = when the *child* generated it.
The two should be very close; the gap is mostly pipe + scheduling latency.

## Implementation notes / issues encountered
1. **Per-pipe line assembly.** A single `read()` from a pipe may return a
   partial line, multiple lines, or a mix. Each pipe therefore has its own
   buffer in the parent; bytes are appended until `\n`, then flushed as one
   tagged output line.
2. **Child 5 multi-line input.** When stdin is itself a pipe (e.g.
   `./project6 < input.txt`), a single `read()` can deliver several lines.
   Child 5 splits on `\n` so each user line becomes its own pipe message
   (instead of being merged and breaking the parent's line tagging).
3. **RNG seeding.** All 5 children fork within microseconds of each other,
   so `srand(getpid() ^ time(NULL))` produced near-identical streams and one
   child dominated output. Switched to mixing in `tv_usec`, a multiplicative
   hash of pid, and child id — children now sleep independently.
4. **30 s deadline for child 5.** Child 5 mustn't block on stdin forever, so
   it uses `select()` on stdin with the *remaining* lifetime as the timeout.
   If the user never types, child 5 still exits exactly at 30 s.
5. **EOF semantics.** Because each pipe's write end exists in exactly one
   process (after the parent closes all write ends and each child closes
   the four pipes it doesn't own), closing the write end on child exit
   immediately delivers EOF to the parent — which is how the parent learns
   the child terminated, without needing `SIGCHLD`.

## Sample I/O
`sample_input.txt`:
```
My first input line
Second user message
Third message from terminal
Fourth line here
Fifth and final input
```
The first few lines of the produced `output.txt`:
```
[parent 0:00.004] 0:00.004: Child 3 message 1
[parent 0:00.004] 0:00.004: Child 5: msg 1 from terminal: My first input line
[parent 0:00.004] 0:00.004: Child 5: msg 2 from terminal: Second user message
[parent 0:00.004] 0:00.004: Child 5: msg 3 from terminal: Third message from terminal
[parent 0:00.004] 0:00.004: Child 5: msg 4 from terminal: Fourth line here
[parent 0:00.004] 0:00.004: Child 5: msg 5 from terminal: Fifth and final input
[parent 0:02.000] 0:02.000: Child 1 message 1
[parent 0:02.004] 0:02.004: Child 2 message 1
...
```
