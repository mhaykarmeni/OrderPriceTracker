// Challenge 01: Order Book — Skeleton Implementation
// This is a naive reference. You can do much better!

#include "solution.h"

namespace hftu {

void OrderBookSTDMap::add_order(uint64_t id, int m_side, int64_t m_price, int64_t m_quantity) {
    m_orders[id] = {m_side, m_price, m_quantity};
    if (m_side == 0) {
        m_bids[m_price] += m_quantity;
    } else {
        m_asks[m_price] += m_quantity;
    }
}

void OrderBookSTDMap::cancel_order(uint64_t id) {
    auto it = m_orders.find(id);
    if (it == m_orders.end()) return;

    auto& order = it->second;
    if (order.m_side == 0) {
        auto bit = m_bids.find(order.m_price);
        if (bit != m_bids.end()) {
            bit->second -= order.m_quantity;
            if (bit->second <= 0) m_bids.erase(bit);
        }
    } else {
        auto ait = m_asks.find(order.m_price);
        if (ait != m_asks.end()) {
            ait->second -= order.m_quantity;
            if (ait->second <= 0) m_asks.erase(ait);
        }
    }
    m_orders.erase(it);
}

int64_t OrderBookSTDMap::best_bid() const {
    return m_bids.empty() ? 0 : m_bids.begin()->first;
}

int64_t OrderBookSTDMap::best_ask() const {
    return m_asks.empty() ? 0 : m_asks.begin()->first;
}

// OrderBookBoostFlatMap

void OrderBookBoostFlatMap::add_order(uint64_t id, int m_side, int64_t m_price, int64_t m_quantity) {
    m_orders[id] = {m_side, m_price, m_quantity};
    if (m_side == 0) {
        m_bids[m_price] += m_quantity;
    } else {
        m_asks[m_price] += m_quantity;
    }
}

void OrderBookBoostFlatMap::cancel_order(uint64_t id) {
    auto it = m_orders.find(id);
    if (it == m_orders.end()) return;

    auto& order = it->second;
    if (order.m_side == 0) {
        auto bit = m_bids.find(order.m_price);
        if (bit != m_bids.end()) {
            bit->second -= order.m_quantity;
            if (bit->second <= 0) m_bids.erase(bit);
        }
    } else {
        auto ait = m_asks.find(order.m_price);
        if (ait != m_asks.end()) {
            ait->second -= order.m_quantity;
            if (ait->second <= 0) m_asks.erase(ait);
        }
    }
    m_orders.erase(it);
}

int64_t OrderBookBoostFlatMap::best_bid() const {
    return m_bids.empty() ? 0 : m_bids.begin()->first;
}

int64_t OrderBookBoostFlatMap::best_ask() const {
    return m_asks.empty() ? 0 : m_asks.begin()->first;
}

// OrderBookSTDFlatMap

void OrderBookSTDFlatMap::add_order(uint64_t id, int m_side, int64_t m_price, int64_t m_quantity) {
    m_orders[id] = {m_side, m_price, m_quantity};
    if (m_side == 0) {
        m_bids[m_price] += m_quantity;
    } else {
        m_asks[m_price] += m_quantity;
    }
}

void OrderBookSTDFlatMap::cancel_order(uint64_t id) {
    auto it = m_orders.find(id);
    if (it == m_orders.end()) return;

    auto& order = it->second;
    if (order.m_side == 0) {
        auto bit = m_bids.find(order.m_price);
        if (bit != m_bids.end()) {
            bit->second -= order.m_quantity;
            if (bit->second <= 0) m_bids.erase(bit);
        }
    } else {
        auto ait = m_asks.find(order.m_price);
        if (ait != m_asks.end()) {
            ait->second -= order.m_quantity;
            if (ait->second <= 0) m_asks.erase(ait);
        }
    }
    m_orders.erase(it);
}

int64_t OrderBookSTDFlatMap::best_bid() const {
    return m_bids.empty() ? 0 : m_bids.begin()->first;
}

int64_t OrderBookSTDFlatMap::best_ask() const {
    return m_asks.empty() ? 0 : m_asks.begin()->first;
}

} // namespace hftu
