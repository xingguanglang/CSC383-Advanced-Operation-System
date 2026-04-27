# COEN 383 — Project 4 Report
## Page Replacement Algorithms Simulator

**Group-N**  
*(Member 1 Name — Lead) · (Member 2) · (Member 3) · (Member 4) · (Member 5)*  
Santa Clara University · Department of Computer Engineering  
Instructor: Prof. Ahmed Ezzat  
Date: ____________

---

## Abstract
*(2–4 sentences. State the problem, what was simulated, and the
headline result — e.g., "Across a 60-second simulation with 150
processes and 100 MB of memory, LRU achieved the highest average hit
rate of XX.XX %, while MFU was the worst at YY.YY %.")*

---

## 1. Introduction *(Member 1)*

### 1.1 Problem Statement
Briefly describe the simulated system:
- 100 MB main memory, 1 MB pages (100 frames)
- 150 processes, sizes ∈ {5, 11, 17, 31} MB
- Service durations ∈ {1, 2, 3, 4, 5} s
- Locality-of-reference reference pattern (70 %)
- Five replacement algorithms compared: FIFO, LRU, LFU, MFU, Random

### 1.2 Goals
- Quantify hit/miss ratio for each algorithm
- Quantify the average number of processes successfully swapped in
  during a 1-minute window
- Identify which algorithm performs best under a locality-rich
  workload and explain *why*.

---

## 2. Methodology *(Member 5)*

### 2.1 Workload Generation
Explain how `generate_workload()` works:
- 150 jobs with random size/duration drawn from uniform distributions
- Arrival times uniform on [0, 60 000] ms
- All jobs sorted into a singly-linked list keyed by arrival time

### 2.2 Memory Model
- Free-frame list of 100 entries (1 MB each)
- A new process is admitted **only when ≥ 4 free frames** are available
- On admission, page-0 is loaded immediately

### 2.3 Locality-of-Reference
Reproduce the algorithm exactly:
```
r = rand() % 11
if r < 7:  next_page = i + Δ, Δ ∈ {-1, 0, +1} (wrap)
else:       next_page ∈ [0, i-2] ∪ [i+2, size-1]
```

### 2.4 Page-Fault Handling
Step-by-step description of `do_reference()` in `simulator.c`.

### 2.5 Experimental Setup
- 5 algorithms × 5 runs = 25 simulations
- All runs use the same generated workload (deep-cloned per run) for
  a fair head-to-head comparison
- Random seed: ____ (record actual seed used)

---

## 3. Algorithms

### 3.1 FIFO *(Member 2)*
- Eviction rule, complexity, expected weakness on looping access
  patterns.
- Pseudocode or refer to `select_victim_fifo` in `fifo_random.c`.

### 3.2 LRU *(Member 3)*
- Eviction rule based on `last_used_ms`
- Why LRU is theoretically optimal among "stack" algorithms when
  locality is high.

### 3.3 LFU *(Member 4)*
- Eviction rule based on `use_count`
- Discuss the *page accumulation* problem (a once-popular page is
  never evicted).

### 3.4 MFU *(Member 4)*
- Inverse of LFU
- Argue when MFU could outperform LFU (newly admitted pages are
  "unproven" and worth keeping).

### 3.5 Random *(Member 2)*
- Baseline; surprisingly competitive when working set ≈ memory size
- Cheapest in implementation cost.

---

## 4. Results & Analysis *(Member 5)*

### 4.1 Hit/Miss Ratio (5-run average)

> Fill in from the program's `OVERALL SUMMARY` table.

| Algorithm | Hit % | Miss % | Avg Swapped-In | Avg Completed |
|-----------|------:|-------:|---------------:|--------------:|
| FIFO      | 68.68 | 31.32  | 150.0          | 143.0         |
| LRU       | 69.38 | 30.62  | 150.0          | 143.0         |
| LFU       | 68.89 | 31.11  | 150.0          | 143.0         |
| MFU       | 68.49 | 31.51  | 150.0          | 143.0         |
| RANDOM    | 69.06 | 30.94  | 150.0          | 143.0         |

*(Numbers above are from one example run with seed 42 — replace with
your actual numbers.)*

### 4.2 Discussion

- **Ranking:** Which algorithm wins, by how much, and is the gap
  statistically meaningful (vs. variance across the 5 runs)?
- **Why LRU is competitive:** The 70 % locality means recently used
  pages are very likely to be used again, exactly the hypothesis LRU
  exploits.
- **Why MFU underperforms:** It evicts hot pages, defeating locality.
- **Why Random does so well:** Working set per process is small (≤ 31
  pages) and total memory (100 frames) is generous → page faults are
  dominated by *cold misses* (first-time access), not eviction
  policy. With enough memory headroom, even random eviction rarely
  evicts a "hot" page.
- **Swapped-in count:** All algorithms admit ~150 processes because
  admission is controlled by free-frame count, which depends on
  arrival/completion timing — *not* on the replacement policy.

### 4.3 Effect of Locality
*(Optional: rerun with a tweaked locality probability and report.)*

### 4.4 Sample Trace (first 100 references)
Paste a 30-line excerpt from the verbose FIFO Run-1 output to
illustrate ENTER/EXIT events and one example eviction.

---

## 5. Conclusion *(Member 1)*

Summarize:
1. Best algorithm under this workload and why
2. Caveats (memory was generous; results may shift under tighter
   constraints)
3. Lessons learned about coordinating a 5-person C project (build
   system, header discipline, data-structure ownership)

---

## 6. Division of Work
See `DIVISION_OF_WORK.md`. Brief recap:

| Member | Modules | Report sections |
|--------|---------|-----------------|
| 1 (Lead) | workload, main, build  | Abstract, §1, §5 |
| 2        | page_table, FIFO, Random | §3.1, §3.5 |
| 3        | LRU                     | §3.2 |
| 4        | LFU, MFU, dispatcher    | §3.3, §3.4 |
| 5        | simulator, stats        | §2, §4 |

---

## 7. How to Build & Reproduce

```bash
make
./pagesim 42 > output.txt 2>&1
```

All outputs in this report come from `output.txt` produced with the
seed shown in §2.5.

---

## Appendix A — Raw Output
*(Attach `output.txt`; or paste the OVERALL SUMMARY block.)*

## Appendix B — Source File Inventory

| File | LOC | Author |
|------|----:|--------|
| common.h        | ~75  | Member 1 |
| workload.c/h    | ~95  | Member 1 |
| main.c          | ~50  | Member 1 |
| page_table.c/h  | ~110 | Member 2 |
| fifo_random.c   | ~35  | Member 2 |
| lru.c           | ~20  | Member 3 |
| lfu_mfu.c       | ~55  | Member 4 |
| simulator.c/h   | ~180 | Member 5 |
| stats.c/h       | ~75  | Member 5 |
| Makefile        | ~40  | Member 1 |
