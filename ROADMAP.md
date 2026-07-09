# MySTL — Completion Roadmap

**Goal:** turn MySTL from a strong-middle work sample into an *outstanding, finished* portfolio project that proves depth in generic programming, modern C++, memory management, template metaprogramming, exception safety, and software architecture.

**This is not a "reach `std` conformance" plan.** It is depth-first. There is an explicit **STOP** marker (after Phase 4). Do not go past it.

**Time estimates** assume ~8–10 focused hrs/week. Difficulty is 1–10.

---

## Progress tracker

| Phase | Theme | Essential hrs | Status |
|------|-------|---------------|--------|
| 0 | Credibility: hygiene, CI, sanitizers | ~10 | ☐ |
| 1 | Correctness & UB | ~14 | ☐ |
| 2 | Allocator contract (the differentiator) | ~35 | ☐ |
| 3 | Make "C++20" true (concepts, ranges, `<=>`) | ~35 | ☐ |
| 4 | One masterpiece + measurement + docs | ~45 | ☐ |
| — | **STOP.** Pivot to next project | — | ☐ |

**Total essential effort ≈ 140 hrs (~4 months part-time).** Optional items add ~30 hrs.

Definition of done for the whole project: **green CI on GCC+Clang+MSVC, clean under ASan/UBSan/TSan, allocator propagation proven with a stateful allocator, `std::ranges` interop demo, a benchmark table with analysis, and per-API docs.**

---

## Phase 0 — Make the repo credible (do first, near-free ROI)

Fix the frame before the content. A reviewer judges the repo in the first 30 seconds.

### 0.1 Repo hygiene — **Essential** · Difficulty 1 · ~1–2 hrs
- [ ] Add `.gitignore` (ignore `build/`, `.cache/`, editor dirs).
- [ ] `git rm -r --cached build/` and remove the committed build tree (stale `src/string.cpp.o` references a non-existent `src/`).
- [ ] Translate all non-English comments to English (`shared_ptr.hpp`, `weak_ptr.hpp`, test files have Russian/Cyrillic).
- [ ] Rename `Readme.md` → `README.md` (canonical casing).
- *Knowledge:* professional git hygiene.

### 0.2 CI matrix — **Essential** · Difficulty 3 · ~4–6 hrs
- [ ] GitHub Actions: build + test on **GCC, Clang, and MSVC**, C++20.
- [ ] Keep `-Wall -Wextra -Wpedantic -Werror` (already in CMake) green on all three.
- [ ] Add a status badge to the README.
- *Why:* proves the code isn't "works on my GCC." MSVC will surface conformance bugs GCC forgives.
- *Knowledge:* CI, toolchain portability, compiler conformance differences.

### 0.3 Sanitizer CI — **Essential (highest value in Phase 0)** · Difficulty 3 · ~3–4 hrs
- [ ] ASan + UBSan job (`-fsanitize=address,undefined -fno-sanitize-recover=all`).
- [ ] Separate **TSan** job that runs the smart-pointer tests (validates the `shared_ptr` thread-safety claim).
- [ ] This job should immediately flag the alignment UB (1.1) and empty-range UB (1.1) — good; that's the point.
- *Knowledge:* sanitizers, race detection, what UB looks like at runtime.

---

## Phase 1 — Correctness & UB (prove the primitives are clean)

You can't claim "exception safety" while the primitives have latent UB.

### 1.1 Fix the six Critical bugs — **Essential** · Difficulty 3 · ~6–8 hrs
- [ ] **`shared_ptr(Y*)` leak-on-throw** — `shared_ptr.hpp:169-177`. If `new control_block_ptr<Y>(p)` throws, `p` leaks. Wrap so `delete p` runs on failure (match `std::shared_ptr`).
- [ ] **Over-alignment UB** — `allocator.hpp:50`. `::operator new(n*sizeof(T))` ignores `alignof(T)`. Use the `std::align_val_t` overload for over-aligned `T`.
- [ ] **Empty-range UB in uninitialized algorithms** — `memory.hpp:81, 95, 110, 125`. `destroy_guard{addressof(*first), ...}` dereferences `first` before the `first != last` check. Move guard init after an empty check (or guard on the loop).
- [ ] **Missing `std::launder`** — `shared_ptr.hpp:142-145`. Launder the `reinterpret_cast` from `control_block_obj` storage.
- [ ] **`String` assignment not exception-safe** — `string.hpp:168-220`. Deallocates `this` heap *before* allocating the copy; if `allocate` throws, `*this` is left dangling. Allocate-before-deallocate.
- [ ] **`Pair` unconstrained converting ctor** — `pair.hpp:19-24`. Constrain `template<U1,U2> Pair(U1&&,U2&&)` with `is_constructible` so it SFINAE-rejects instead of hard-erroring / shadowing the copy ctor.
- *Knowledge:* exception-safety ordering, alignment, object lifetime/`launder`, SFINAE-constrained ctors.

### 1.2 Adversarial test toolkit (reusable header) — **Essential** · Difficulty 4 · ~6–8 hrs
- [ ] `tests/support/` header with: `ThrowingType<N>` (throw on Nth copy/move), a move-only type, a non-default-constructible type.
- [ ] `InstrumentedAllocator`: counts alloc/dealloc, tags blocks with an allocator ID, **asserts on cross-allocator free** (this is what makes Phase 2 provable).
- [ ] Retrofit existing exception tests to use the toolkit.
- *Why:* great test *infrastructure* > more test *cases*. Enables Phase 2 proofs.
- *Knowledge:* test design, exception-safety verification, allocator instrumentation.

---

## Phase 2 — The allocator contract, done properly (your biggest differentiator)

Almost no candidate finishes this. It's your promotion argument. Your `allocator_traits` already *defines* the propagation traits (`memory.hpp:216-219`) — you just never *consult* them.

### 2.1 Honor propagation traits everywhere — **Essential** · Difficulty 8 · ~20–30 hrs
For **every** container's `operator=` (copy + move) and `swap`, consult:
- [ ] `propagate_on_container_copy_assignment` (POCCA) in copy-assign.
- [ ] `propagate_on_container_move_assignment` (POCMA) in move-assign — the hard case: when POCMA is false **and** allocators are unequal, you must **element-wise move into your own storage**, not pointer-steal.
- [ ] `propagate_on_container_swap` (POCS) + `is_always_equal` in `swap`.
- [ ] Make move-assign `noexcept` conditional on the correct traits.
- [ ] Containers to update: `Vector`, `String`, `Deque`, `List`, `ForwardList`, `RBTree` (covers all ordered), `HashTable` (covers all unordered).
- [ ] Prove each with `InstrumentedAllocator` tests (no cross-allocator frees; correct propagation observed).
- *Knowledge:* the deepest part of the container/allocator spec; why move-assign is `noexcept`-conditional.

### 2.2 A real stateful allocator + proofs — **Essential** · Difficulty 6 · ~12–16 hrs
- [ ] Implement a **monotonic/arena allocator** (bump-pointer, no per-object free, owns a buffer).
- [ ] Make it stateful and *not* always-equal so it exercises 2.1's unequal-allocator paths.
- [ ] Tests: containers backed by the arena; verify propagation behavior and that nothing frees through the wrong allocator.
- [ ] Support `uses_allocator` construction where relevant.
- *Knowledge:* memory arenas, allocator statefulness, `propagate_on_*` observable behavior.

### 2.3 Search-before-allocate in backbones — **Optional** · Difficulty 3 · ~2–3 hrs
- [ ] `RBTree::emplace_unique` (`rb_tree.hpp:572`) and `HashTable::emplace_unique` (`hash_table.hpp:571`) allocate+construct before the dedup check. Search first; allocate only on success.
- *Knowledge:* allocation-cost awareness, node reuse.

---

## Phase 3 — Make the "C++20" claim true (modern-language fluency)

The repo is idiomatic C++17 flying a C++20 flag. Fix that with one deep, correct demonstration of each headline feature.

### 3.1 Concept-constrain the public template surface — **Essential** · Difficulty 6 · ~15–20 hrs
- [ ] Replace `enable_if`/`is_integral` disambiguation (e.g. `vector.hpp:89`, `deque.hpp:311`) with `std::input_iterator` / `std::sentinel_for`.
- [ ] Add custom concepts: `Hashable`, an `Allocator`-like concept, `Comparator`.
- [ ] Add `requires`-clauses that improve error messages on misuse.
- *Why:* concepts are *the* headline C++20 feature; constrained templates with clean diagnostics are what "Modern C++" means now.
- *Knowledge:* concepts, `requires`, subsumption, constraint-based overload resolution.

### 3.2 Model `std::ranges` concepts + interop demo — **Essential (showpiece)** · Difficulty 7 · ~12–16 hrs
- [ ] Give iterators `iterator_concept` (not just legacy tags); add sentinels where useful.
- [ ] Ensure `mystl::Vector`, `Deque`, `List`, `Map` satisfy the right `std::ranges` concepts.
- [ ] **Demo (put it in the README):** `std::ranges::sort(myVector)`, `myMap | std::views::filter(...)`, etc., compiling and running.
- *Why:* "my containers plug into `std::ranges`" proves your iterator abstractions are *conformant*, not merely functional. Best single demo in the project.
- *Knowledge:* C++20 iterator concepts vs legacy tags, sentinels, ranges CPOs.

### 3.3 `operator<=>` + `operator==` — **Optional (recommended, cheap)** · Difficulty 4 · ~6–8 hrs
- [ ] Replace the six hand-rolled comparison operators on containers/`Pair`/`Tuple`/`array` with `<=>` + `==`.
- [ ] Pick correct ordering categories (`strong_ordering` for `int`-like, `partial_ordering` for float-containing).
- *Knowledge:* three-way comparison, ordering categories, rewritten candidates.

### 3.4 Deduction guides (CTAD) — **Optional** · Difficulty 4 · ~4–6 hrs
- [ ] Add guides so `mystl::Vector v{1,2,3};` deduces `Vector<int>`; same for `Pair`, `Tuple`, associative containers from iterator ranges.
- *Knowledge:* CTAD, deduction guides.

---

## Phase 4 — One masterpiece + measurement + docs

Depth beats breadth. Finish **one** container completely, then prove your performance claims.

### 4.1 Make `String` a masterpiece — **Essential (depth showcase)** · Difficulty 7 · ~20–25 hrs
- [ ] Correct SSO behavior at the boundary (size 15↔16); add tests for self-append and aliasing.
- [ ] API: `append`, `operator+`, `operator+=`, `substr`, `find`, `rfind`, `replace`, `resize`, `insert`, `erase`, `operator=(const char*)`, `c_str`/`data` non-const.
- [ ] Templatize on a `char_traits`-style policy.
- [ ] `constexpr`-enable (C++20).
- [ ] `std::hash` / `mystl::hash` integration (closes a known README gap).
- [ ] Full exception safety + exhaustive tests.
- *Why:* one complete, `constexpr`, exception-safe, `char_traits`-templated SSO string > eight half-containers.
- *Knowledge:* SSO internals, `char_traits` policy, `constexpr` containers, aliasing/self-reference safety.

### 4.2 Benchmark harness + analysis — **Essential (2nd-highest value)** · Difficulty 5 · ~12–16 hrs
- [ ] Google Benchmark comparing MySTL vs `libstdc++`/`libc++`: `Vector::push_back`, `Map` insert/find, `unordered_map` insert/find, `sort`, `String::append`.
- [ ] Publish a results table in the README.
- [ ] **Write the analysis** — *where and why* you differ (e.g. power-of-2 `%` hashing quality, realloc growth factor, node layout). The analysis matters more than the numbers.
- *Knowledge:* micro-benchmark pitfalls, cache effects, `perf`, honest performance analysis.

### 4.3 Documentation pass — **Essential** · Difficulty 3 · ~8–10 hrs
- [ ] Per-API **complexity** + **exception-guarantee** annotations (at least for the major containers).
- [ ] A "conformance vs `std`" table (what's implemented / differs / omitted, on purpose).
- [ ] A short **design-decisions** writeup: why backbone-sharing, why `move_if_noexcept`, the allocator choices, the SSO design.
- [ ] Fix the stale README line claiming unordered containers "duplicate much of their implementation" — you built the shared `HashTable` backbone.
- *Knowledge:* technical writing, honest self-assessment.

---

## 🛑 STOP HERE

After Phase 4, **adding more STL features has sharply diminishing returns.**

- More containers/algorithms (`List::splice`, another `<algorithm>`, unordered `bucket()`) teach nothing new — same node splicing, same iterator plumbing.
- Employers don't read past the first few components; a 6th container doesn't raise hire probability.
- Do **not** chase `std` conformance — multi-year trap, no portfolio payoff.

**Only exceptions:** a *specific* feature that demos a *new* technique, done as a one-off deep dive:
- `enable_shared_from_this` (the weak-from-this trick), OR
- a small-buffer-optimized `function` (type erasure).

If you catch yourself copy-pasting the 4th associative wrapper: **stop, collapse the 8 wrappers with a CRTP base (~3 hr refactor, shows DRY judgment), then leave.**

---

## Next portfolio project (start after STOP)

Prove a *different* axis so you're not one-dimensional.

**► Top pick: a lock-free / concurrent data-structure library.**
MPMC queue + a reclamation scheme (hazard pointers or epoch-based) + lock-free stack, with stress tests under TSan and a linearizability check.
- *Why:* natural escalation from your `shared_ptr` atomics; highest-paid, hardest-to-fake C++ systems skill; almost nobody has a credible concurrency project. Complements MySTL (memory+generics → concurrency+reclamation).

**► Alternatives:** C++20 coroutine async-I/O library (`io_uring`/epoll + `task<T>` + executor); or a `std::function`-style type-erasure + small serialization layer; or a standalone arena allocator + memory profiler.

Two projects — *generics/memory* (MySTL) and *concurrency* — make a portfolio that reads "systems engineer," not "did the STL."

---

## Learning roadmap — strong C++ systems engineer

Sequenced; each tier assumes the previous.

**Tier 1 — Language mastery** (you're mostly here)
Value categories, move/forwarding, RAII, Rule of 0/3/5; templates → SFINAE → **Concepts**; variadics, folds, CRTP, EBO, type erasure; exception-safety guarantees as design discipline.
*Read:* Meyers *Effective Modern C++*; Vandevoorde/Josuttis *C++ Templates* (2e). *(MySTL Phases 1–3.)*

**Tier 2 — Memory & the machine**
Allocators, arenas, PMR, alignment, object lifetime/`launder`; cache hierarchy, false sharing, data-oriented design, `perf`; the C++ memory model (`memory_order`, acquire/release). *(MySTL Phases 2 & 4.2.)*

**Tier 3 — Concurrency & parallelism**
Atomics deeply, lock-free structures, hazard pointers/RCU/epoch, ABA; executors, thread pools, C++20 coroutines, structured concurrency; TSan, model checkers, linearizability testing.
*Read:* Williams *C++ Concurrency in Action* (2e). *(Your 2nd project.)*

**Tier 4 — Systems & performance engineering**
OS internals (virtual memory, syscalls, `mmap`, `io_uring`/epoll, NUMA); toolchain (inlining, devirtualization, LTO, reading asm, ABI, linkers); profiling habit + benchmark methodology.
*Read:* Pikus *The Art of Writing Efficient Programs*; Agner Fog's manuals.

**Tier 5 — Architecture & judgment** (promotion tier)
API/ABI design, versioning, compatibility; CMake mastery + Bazel exposure; writing design docs/RFCs; reading real systems code (libc++, folly, abseil).

**12–18 month plan:**
1. **Months 1–4:** MySTL Phases 0–4. Stop at the marker.
2. **Months 5–10:** concurrency project + Williams' book in parallel.
3. **Months 11–14:** read + land a *small* merged PR to a real C++ library (libc++/folly/etc.) — outweighs any personal project.
4. **Ongoing:** one benchmark-and-writeup post per project. Writing about your work is the reputation multiplier.

---

*The three moments that make an interviewer lean forward: allocator propagation proven with a stateful allocator (2.1–2.2), `std::ranges` interop (3.2), and the benchmark analysis (4.2). Everything else supports those. Chase depth, proof, and the ability to explain tradeoffs — not conformance.*
