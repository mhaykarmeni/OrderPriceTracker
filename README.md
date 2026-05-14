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

## Benchmark

- **Workload mix:** 60% adds, 20% cancels, 10% `best_bid`, 10% `best_ask`
- **Metric:** x86-64 CPU cycles per operation via `rdtscp` (lower is better)
