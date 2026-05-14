#pragma once

#include <cstdint>
#include <flat_map>
#include <map>
#include <unordered_map>
#include <boost/container/flat_map.hpp>

namespace hftu {

class OrderBookSTDMap {
public:
    // Add a new order. m_side: 0=buy(bid), 1=sell(ask)
    void add_order(uint64_t id, int m_side, int64_t m_price, int64_t m_quantity);

    // Cancel an order by ID. No-op if ID doesn't exist.
    void cancel_order(uint64_t id);

    // Return highest bid m_price, or 0 if no bids.
    int64_t best_bid() const;

    // Return lowest ask m_price, or 0 if no asks.
    int64_t best_ask() const;

private:
    struct Order {
        int m_side;
        int64_t m_price;
        int64_t m_quantity;
    };

    std::unordered_map<uint64_t, Order> m_orders;
    std::map<int64_t, int64_t, std::greater<>> m_bids; // m_price -> total qty, descending
    std::map<int64_t, int64_t> m_asks;                  // m_price -> total qty, ascending
};

class OrderBookBoostFlatMap {
public:
    void add_order(uint64_t id, int m_side, int64_t m_price, int64_t m_quantity);
    void cancel_order(uint64_t id);
    int64_t best_bid() const;
    int64_t best_ask() const;

private:
    struct Order {
        int m_side;
        int64_t m_price;
        int64_t m_quantity;
    };

    std::unordered_map<uint64_t, Order> m_orders;
    boost::container::flat_map<int64_t, int64_t, std::greater<>> m_bids;
    boost::container::flat_map<int64_t, int64_t> m_asks;
};

class OrderBookSTDFlatMap {
public:
    void add_order(uint64_t id, int m_side, int64_t m_price, int64_t m_quantity);
    void cancel_order(uint64_t id);
    int64_t best_bid() const;
    int64_t best_ask() const;

private:
    struct Order {
        int m_m_side;
        int64_t m_price;
        int64_t m_quantity;
    };

    std::unordered_map<uint64_t, Order> m_orders;
    std::flat_map<int64_t, int64_t, std::greater<>> m_bids;
    std::flat_map<int64_t, int64_t> m_asks;
};

}
