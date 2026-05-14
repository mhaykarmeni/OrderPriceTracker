# Limit Order Book

## Problem

Implement a limit order book that supports the following operations:

| Operation | Signature | Description |
|-----------|-----------|-------------|
| Add order | `add_order(id, side, price, quantity)` | Insert a new order |
| Cancel order | `cancel_order(id)` | Remove an order by ID |
| Best bid | `best_bid()` | Return the highest bid price (or `0` if empty) |
| Best ask | `best_ask()` | Return the lowest ask price (or `0` if empty) |

## Constraints

- **Prices:** integers in range `[1, 1,000,000]`
- **Quantities:** integers in range `[1, 10,000]`
- **Order IDs:** unique `uint64_t` values
- **Side:** `0` = buy (bid), `1` = sell (ask)
- **Capacity:** up to 1,000,000 active orders at once

## Building

### Prerequisites

`std::flat_map` requires GCC 15.2+. On Ubuntu 24.04 the standard repos only ship GCC 14, so you need the Toolchain PPA:

```bash
sudo add-apt-repository ppa:ubuntu-toolchain-r/test
sudo apt-get update
sudo apt-get install -y gcc-15 g++-15
```

Boost is also required (`boost::container::flat_map`):

```bash
sudo apt-get install -y libboost-dev
```

### Compile

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=g++-15
cmake --build build -j$(nproc)
```

### Run benchmarks

```bash
./build/compare    # all four implementations side-by-side
./build/benchmark  # certified single-implementation benchmark
```

## Implementations

All four classes share the same public interface via CRTP. `OrderBookBase<Derived>` is a marker base; `OrderBookMapBase<Derived>` provides the shared logic for the three map-based variants.

### OrderBookSTDMap

Uses `std::map` (red-black tree) for the bid and ask price levels, and `std::unordered_map` for order ID lookup. Simple and correct with O(log n) add/cancel and O(1) best bid/ask. Serves as the reference baseline.

### OrderBookBoostFlatMap

Replaces `std::map` with `boost::container::flat_map` — a sorted contiguous array. Better cache locality than a tree for small books, but O(n) shifts on insert/cancel make it degrade badly at high order counts.

### OrderBookSTDFlatMap

Identical in design to `OrderBookBoostFlatMap` but uses the C++23 standard `std::flat_map`. Requires GCC 15+ / libstdc++ 15. Performance characteristics are the same as the Boost variant.

### OrderBookFlatHMap (Aggressive)

Designed for minimum latency. Three key ideas:

**1. Direct-indexed quantity array**
`m_qty[side][price]` is a plain `int64_t` array of 1,000,001 entries. Adding or cancelling a quantity at a price level is a single array write — no tree traversal, no hash lookup, no allocation.

**2. 3-level bitset for best bid/ask**
Tracks which price levels are non-empty using a hierarchy of 64-bit words:
- **L1** — one bit per price (15,626 words covering prices 0–1,000,000)
- **L2** — one bit per L1 word (245 words)
- **L3** — one bit per L2 word (4 words)

`best_bid` scans L3 (4 words max) with `__builtin_clzll`, then descends to L2 and L1 in two more steps — at most 3 cache lines touched regardless of how many orders are active.

**3. Open-addressing flat hash map**
Order ID → (price, qty, side) is stored in a 2M-slot array with Fibonacci hashing and backward-shift deletion (no tombstones). Probe chains stay short, and the entire hot path fits in L1/L2 cache.

## Benchmark

- **Workload mix:** 60% adds, 20% cancels, 10% `best_bid`, 10% `best_ask`
- **Metric:** x86-64 CPU cycles per operation via `rdtscp` (lower is better)

### Results

**Compiler:** GCC 15.2.0, `-O3 -march=native`
**Workload:** 100,000 operations per run, 10 iterations

| Implementation | AMD Ryzen 7 7730U (cycles/op) | Intel Core Ultra 7 155U WSL2 (cycles/op) |
|---|---|---|
| `OrderBookFlatHMap` | **70** | **87** |
| `OrderBookSTDMap` | 385 | 418 |
| `OrderBookSTDFlatMap` | 1,908 | 2,897 |
| `OrderBookBoostFlatMap` | 2,115 | 2,973 |

**AMD Ryzen 7 7730U:** 16 logical cores, ~2.0 GHz, 8 GiB RAM, L1d 256 KiB × 8, L2 4 MiB × 8, L3 16 MiB
**Intel Core Ultra 7 155U (WSL2):** 7 logical cores, ~2.7 GHz, 20 GiB RAM, L1d 48 KiB × 4, L2 2 MiB × 4, L3 12 MiB

The flat map variants are slowest due to O(n) shifts on insert/cancel across up to 1,000,000 price levels. `std::map` handles those in O(log n) via a red-black tree. The aggressive implementation avoids tree overhead entirely with a direct-indexed quantity array and a 3-level bitset for O(1) `best_bid`/`best_ask`.
