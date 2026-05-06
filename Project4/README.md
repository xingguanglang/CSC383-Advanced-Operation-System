# COEN 383 Project-4: Page Replacement Simulator

Santa Clara University · Department of Computer Engineering  
Instructor: Ahmed Ezzat · Group-N

---

## 1. Project Overview

This project simulates a paging system with **100 MB of main memory**
(100 × 1 MB page frames) running **150 randomly generated processes**.
Each process references memory pages every 100 ms, following a
**locality-of-reference** model. Five page-replacement algorithms are
compared:

| Algorithm | Description |
|-----------|-------------|
| **FIFO**   | Evict the page that was loaded earliest |
| **LRU**    | Evict the page that has not been used for the longest time |
| **LFU**    | Evict the page with the *lowest* reference count |
| **MFU**    | Evict the page with the *highest* reference count |
| **RANDOM** | Evict a random resident page |

Each algorithm is run **5 times for 60 seconds**; we report the average
hit/miss ratio and the average number of processes successfully
swapped in.

---

## 2. File Structure

```
page_replacement_sim/
├── Makefile            # Build script
├── README.md           # This file
├── DIVISION_OF_WORK.md # Per-member responsibilities
├── REPORT_TEMPLATE.md  # Final report template
│
├── common.h            # Shared constants & data structures (Member 1)
├── workload.{c,h}      # Random workload generator        (Member 1)
├── main.c              # Top-level driver                  (Member 1)
│
├── page_table.{c,h}    # Frame allocation & memory map     (Member 2)
├── replacement.h       # Algorithm interface               (shared)
├── fifo_random.c       # FIFO + RANDOM algorithms          (Member 2)
│
├── lru.c               # LRU algorithm                     (Member 3)
│
├── lfu_mfu.c           # LFU + MFU + dispatcher            (Member 4)
│
├── simulator.{c,h}     # Main simulation loop & locality   (Member 5)
└── stats.{c,h}         # Statistics aggregation & output   (Member 5)
```

---

## 3. Build & Run

```bash
make           # build ./pagesim
./pagesim      # run with random seed
./pagesim 42   # run with fixed seed 42 (reproducible)
make run       # equivalent to ./pagesim 42
make clean
```

To capture the full output for the report:
```bash
./pagesim 42 > output.txt 2>&1
```

---

## 4. Output Sections

1. **Workload table** — first 20 generated processes
2. **Verbose trace** — full ENTER/EXIT events + first 100 page-reference
   records (printed only for FIFO Run 1 to control log size)
3. **Per-run statistics** — hits, misses, hit%, swapped-in, completed
4. **Per-algorithm 5-run average**
5. **Overall comparison table**

---

## 5. Notes on Implementation

* All five algorithms share the same generated workload to ensure
  fair comparison (the workload is generated once in `main` and
  deep-cloned per run).
* Time is advanced in 100 ms ticks; every running process issues one
  reference per tick.
* The locality-of-reference rule strictly follows the spec:
  * `r ∈ [0,7) ⇒ Δi ∈ {-1, 0, +1}` with wrap-around
  * `r ∈ [7,10] ⇒ |Δi| ≥ 2`
* Memory map uses one character per 1 MB frame:
  `'.'` = free, capital letter = process A–Z, lowercase = P26+
* A process is admitted only when at least 4 free frames exist
  (project requirement).
