#pragma once

#include <cstdint>
#include <flat_map>
#include <map>
#include <unordered_map>
#include <boost/container/flat_map.hpp>


template <typename Derived>
class OrderBookBase {
public:
    void add_order(uint64_t id, int side, int64_t price, int64_t quantity) {
        m_orders[id] = {side, price, quantity};
        if (side == 0) self().m_bids[price] += quantity;
        else           self().m_asks[price] += quantity;
    }

    void cancel_order(uint64_t id) {
        auto it = m_orders.find(id);
        if (it == m_orders.end()) return;

        const auto& order = it->second;
        if (order.m_side == 0) {
            auto bit = self().m_bids.find(order.m_price);
            if (bit != self().m_bids.end()) {
                bit->second -= order.m_quantity;
                if (bit->second <= 0) self().m_bids.erase(bit);
            }
        } else {
            auto ait = self().m_asks.find(order.m_price);
            if (ait != self().m_asks.end()) {
                ait->second -= order.m_quantity;
                if (ait->second <= 0) self().m_asks.erase(ait);
            }
        }
        m_orders.erase(it);
    }

    int64_t best_bid() const {
        return self().m_bids.empty() ? 0 : self().m_bids.begin()->first;
    }

    int64_t best_ask() const {
        return self().m_asks.empty() ? 0 : self().m_asks.begin()->first;
    }

protected:
    struct Order {
        int     m_side;
        int64_t m_price;
        int64_t m_quantity;
    };

    std::unordered_map<uint64_t, Order> m_orders;

private:
    Derived&       self()       { return static_cast<Derived&>(*this); }
    const Derived& self() const { return static_cast<const Derived&>(*this); }
};

class OrderBookSTDMap : public OrderBookBase<OrderBookSTDMap> {
    friend class OrderBookBase<OrderBookSTDMap>;
    std::map<int64_t, int64_t, std::greater<>> m_bids;
    std::map<int64_t, int64_t>                 m_asks;
};

class OrderBookBoostFlatMap : public OrderBookBase<OrderBookBoostFlatMap> {
    friend class OrderBookBase<OrderBookBoostFlatMap>;
    boost::container::flat_map<int64_t, int64_t, std::greater<>> m_bids;
    boost::container::flat_map<int64_t, int64_t>                 m_asks;
};

class OrderBookSTDFlatMap : public OrderBookBase<OrderBookSTDFlatMap> {
    friend class OrderBookBase<OrderBookSTDFlatMap>;
    std::flat_map<int64_t, int64_t, std::greater<>> m_bids;
    std::flat_map<int64_t, int64_t>                 m_asks;
};
