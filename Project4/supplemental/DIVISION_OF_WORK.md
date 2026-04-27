# Division of Work — COEN 383 Project-4 (Group-N)

> 5-person team. Each member owns specific source files and is responsible
> for testing their own module before integration. Member 1 (Team Lead)
> coordinates the merge and runs the final experiments.

---

## Member 1 — Team Lead / Workload & Integration
**Files owned:** `common.h`, `workload.c/h`, `main.c`, `Makefile`, `README.md`

**Responsibilities**
1. Define shared data structures (`Process`, `Frame`, `RunStats`,
   `AlgorithmType`) in `common.h`.
2. Implement `generate_workload()` — 150 processes with random
   size {5, 11, 17, 31} MB and duration {1..5} s, sorted by arrival time.
3. Build `main.c`: orchestrate 5 algorithms × 5 runs, share one
   workload across all runs, dispatch to `run_simulation()`.
4. Maintain build system (`Makefile`) and project documentation.
5. Coordinate weekly merges and resolve git conflicts.
6. Write **§1 Introduction** and **§5 Conclusion** of the report.

**Deliverable check**
- [ ] `make` builds with no warnings
- [ ] `./pagesim 42` produces deterministic output

---

## Member 2 — Frame Manager + FIFO + Random
**Files owned:** `page_table.c/h`, `fifo_random.c`, `replacement.h`

**Responsibilities**
1. Implement the global frame array `g_frames[100]` and helpers:
   `init_frames`, `find_page`, `allocate_frame`, `free_frame`,
   `free_process_frames`, `free_frame_count`.
2. Implement `build_memory_map()` — produces the
   `<AAbbbACCAAbbbbbbbbCCC....>` style string required by the spec.
3. Implement `select_victim_fifo()` — pick frame with smallest
   `load_time_ms`.
4. Implement `select_victim_random()` — uniform random pick from
   resident frames.
5. Write **§3.1 (FIFO)** and **§3.5 (Random)** of the report.

**Deliverable check**
- [ ] Memory map prints exactly 100 chars
- [ ] FIFO consistently evicts oldest-loaded frame in unit test

---

## Member 3 — LRU
**Files owned:** `lru.c`

**Responsibilities**
1. Implement `select_victim_lru()` — pick frame with smallest
   `last_used_ms`.
2. Coordinate with Member 5 to make sure `last_used_ms` is updated
   on every HIT and on initial load (verified in `simulator.c`).
3. Build a small unit test demonstrating LRU's advantage on
   sequential-access patterns (optional, for the report appendix).
4. Write **§3.2 (LRU)** of the report — discuss complexity, expected
   behavior under the locality-of-reference workload.

**Deliverable check**
- [ ] LRU achieves the highest (or tied-highest) hit rate in the final
      experiment table

---

## Member 4 — LFU + MFU
**Files owned:** `lfu_mfu.c`

**Responsibilities**
1. Implement `select_victim_lfu()` — minimum `use_count`
   (tie-break by older `load_time_ms`).
2. Implement `select_victim_mfu()` — maximum `use_count`
   (tie-break by older `load_time_ms`).
3. Implement the `select_victim()` dispatcher (switch on
   `AlgorithmType`).
4. Coordinate with Member 5 to ensure `use_count` is incremented on
   HIT and reset to 1 on initial load.
5. Write **§3.3 (LFU)** and **§3.4 (MFU)** of the report — explain why
   MFU sometimes outperforms LFU and vice-versa.

**Deliverable check**
- [ ] Manual trace verifies LFU evicts the lowest-count frame
- [ ] LFU and MFU produce *different* eviction patterns on the same input

---

## Member 5 — Simulator Engine + Statistics
**Files owned:** `simulator.c/h`, `stats.c/h`

**Responsibilities**
1. Implement `next_page_with_locality(i, size)` — exact spec algorithm:
   * `r ∈ [0,7)` → Δi ∈ {-1, 0, +1} (wrap)
   * `r ∈ [7,10]` → |Δi| ≥ 2
2. Implement the main loop in `run_simulation()`:
   * Time tick 100 ms
   * Admit arrivals when ≥ 4 free frames
   * Each running process issues one reference per tick
   * On miss with no free frame → call `select_victim()`
   * On process completion → release all its frames
3. Implement verbose trace: ENTER/EXIT records + first 100 reference
   records with full memory map.
4. Implement `print_run_stats`, `print_alg_summary`,
   `print_overall_summary`.
5. Write **§2 Methodology** and **§4 Results & Analysis** of the report
   (this includes the comparison table and discussion).

**Deliverable check**
- [ ] Hit rate is in plausible range (≈ 65–75 % given 70 % locality)
- [ ] All 5 algorithms admit roughly the same number of processes
      (workload-bound, not algorithm-bound)
- [ ] Output file under 5 MB

## 
