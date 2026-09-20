# Fuzzing

PoolAllocator is fuzzed via [ClusterFuzzLite](https://google.github.io/clusterfuzzlite/),
running on every pull request that touches the fuzzed files, plus a
longer scheduled batch run every night.

## What's covered

**`fuzz_pool.cpp`** is a differential fuzzer: it drives a `Pool` and an
independent shadow model with the same sequence of operations, and
compares them after every single operation (not just at the end), so a
failing input localizes to the exact operation that broke an invariant.

The model is deliberately *exact*, not approximate. It tracks the free
list as a LIFO stack of block indices plus the watermark, so every
pointer the pool returns is compared against the one the model says
must come next — not merely "some valid block". That is what pins down
the documented behavior (the free list is drained before the watermark
is touched, freed blocks are reused LIFO, virgin blocks are handed out
in address order) rather than only checking that nothing crashed.

Every live block is also filled with a per-block byte pattern (or, for
`create()`d blocks, a `Probe` object plus a patterned tail) and
re-verified after every operation. The whole pool is a single
allocation, so ASan cannot see one block overlapping another — this
pattern check is what catches a free-list node being written into a
block that is still allocated, or a block being handed out twice.

Specifically exercised:

- **`allocate()` / `deallocate()`** and **`allocateBatch()` /
  `deallocateBatch()`**, including batches larger than the pool
  (exhaustion mid-batch), zero-length batches, and batches that mix
  live blocks with pointers the pool must skip. Slots past the returned
  count must be left untouched.
- **The "not owned" contract.** `nullptr`, stack addresses, interior
  pointers, one-past-the-end, just-before-the-start, and blocks
  belonging to a *different* pool must all be silently ignored by
  `deallocate()`, `deallocateBatch()` and `destroy()` — with no change
  to any state or statistic — and rejected by `owns()`. `owns()` is
  also checked to be true for every block start (allocated, free and
  virgin alike) and false for any address inside a block.
- **`create()`'s exception path.** `Probe`'s constructor throws on
  request. The harness checks the block was handed straight back (net
  effect: on top of the free list, with both an allocation and a
  deallocation counted), that exhaustion returns `nullptr` *before* the
  constructor runs, and that no object is ever leaked or destroyed
  twice.
- **`destroy()`** runs the destructor exactly once, then frees the
  block.
- **`reset()`**, including statistics returning to zero and every block
  becoming virgin again.
- **Move construction and move assignment** — into a live pool (whose
  arena must be released; LSan flags a leak), into a moved-from pool,
  and self-move-assignment — plus that a moved-from pool is a valid,
  zero-capacity pool that hands out `nullptr` and ignores everything.
- **Both `Pool<false>` and `Pool<true>`.** For the latter, every
  `Stats` counter is compared against the model after every operation.
- **Configurations chosen to reach every internal path**: strides that
  are a power of two (`owns()` uses a mask) and ones that aren't (it
  uses `%`), and block sizes that aren't a multiple of the alignment
  (stride rounding).

Assertions are forced on in the harness (`NDEBUG` is undefined at the
top of the file), so `AP_PRE`/`AP_ASSERT` contract violations abort
instead of silently becoming undefined behavior, regardless of the build
flags in use.

Built and run under both AddressSanitizer and UndefinedBehaviorSanitizer.

## What's deliberately NOT covered yet

- **Caller-side contract violations.** Double-frees, freeing a block
  that isn't currently allocated, and use-after-free are the caller's
  responsibility; `AP_ASSERT` only catches an *immediate* repeat of the
  same free, in debug builds. The harness never does any of these, since
  a failure there would be the fuzzer misusing the API rather than a
  bug in the pool.
- **Very large pools and alignments.** Pools are capped at 33 blocks of
  at most 64 bytes with alignments up to 64. Small pools reach
  exhaustion within a few bytes of input, which is what makes the
  free-list/watermark interplay fuzzable at all. The `stride *
  blockCount` overflow precondition in `allocateStorage()` is not
  reachable from here without huge allocations.
- **Threading.** `Pool` is not thread-safe, so there is nothing to fuzz.

## Known sharp edges

Two behaviors the harness deliberately steers around, because they are
contract edges rather than bugs a differential test can meaningfully
check. Both are reproducible in a few lines.

- **Alignment below `alignof(void*)`.** The constructor only requires
  `alignment` to be a power of two, but every freed block has a
  `FreeNode` written into it. With, say, `Pool(9, 4, 1)` the stride is 9,
  so the second block sits at a misaligned address and UBSan reports a
  misaligned member access on the first `deallocate()`. The harness only
  builds pools with `alignment >= 8`. A precondition such as
  `AP_PRE(alignment >= alignof(FreeNode))` in
  `validateAndComputeStride()` (or rounding the alignment up to it)
  would close this; if you add one, extend the alignment table in
  `Config::read()` to cover the smaller values.
- **`create<T>()` on a moved-from pool.** A move zeroes `blockSize_`, so
  `AP_PRE(sizeof(T) <= blockSize_)` fires in debug builds — while
  `allocate()` on the very same pool just returns `nullptr`. The harness
  skips `create()` on moved-from pools. If moved-from pools are meant to
  behave uniformly, the precondition could be checked only after the
  pool is known to be non-empty, or `create()` could early-out on it.

## Running locally

```bash
git clone --recursive https://github.com/google/oss-fuzz.git
cd oss-fuzz
python infra/helper.py build_fuzzers --sanitizer address PoolAllocator /path/to/PoolAllocator
python infra/helper.py run_fuzzer PoolAllocator fuzz_pool
```

Or, without OSS-Fuzz's tooling, directly with clang:

```bash
clang++ -std=c++20 -fsanitize=fuzzer,address \
  -Iinclude \
  fuzz/fuzz_pool.cpp \
  -o fuzz_pool

./fuzz_pool
```

Add `-fsanitize=fuzzer,undefined` instead to run under UBSan.

## Reproducing a crash

ClusterFuzzLite uploads the failing input as a workflow artifact when
a run fails. Download it, then:

```bash
./fuzz_pool path/to/crash-<hash>
```

This replays that exact byte sequence through
`LLVMFuzzerTestOneInput()` once, deterministically — no sanitizer flags
needed beyond however the binary was already built.

A failed model check prints the invariant and the line it lives on
(`FUZZ_CHECK failed: <condition> (fuzz_pool.cpp:<line>)`) before
aborting, so the log alone usually says which behavior diverged.

## Adding a new harness

1. Add `fuzz/fuzz_<target>.cpp` with an `extern "C" int
   LLVMFuzzerTestOneInput(const uint8_t*, size_t)` entry point.
2. Add the matching compile + link block to `.clusterfuzzlite/build.sh`.
3. No workflow changes needed — `cflite_pr.yml`/`cflite_batch.yml`
   build and run every binary `build.sh` produces in `$OUT`.
