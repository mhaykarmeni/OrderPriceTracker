#pragma once

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <flat_map>
#include <map>
#include <unordered_map>
#include <boost/container/flat_map.hpp>

// ── CRTP marker base ───────────────────────────────────────────────────────
template <typename Derived>
class OrderBookBase {};

// ── Shared implementation for map-based variants ───────────────────────────
template <typename Derived>
class OrderBookMapBase : public OrderBookBase<Derived> {
protected:
    struct Order {
        int     m_side;
        int64_t m_price;
        int64_t m_quantity;
    };
    std::unordered_map<uint64_t, Order> m_orders;

    Derived&       self()       { return static_cast<Derived&>(*this); }
    const Derived& self() const { return static_cast<const Derived&>(*this); }

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
};

// ── Concrete map-based classes ─────────────────────────────────────────────

class OrderBookSTDMap : public OrderBookMapBase<OrderBookSTDMap> {
    friend class OrderBookMapBase<OrderBookSTDMap>;
    std::map<int64_t, int64_t, std::greater<>> m_bids;
    std::map<int64_t, int64_t>                 m_asks;
};

class OrderBookBoostFlatMap : public OrderBookMapBase<OrderBookBoostFlatMap> {
    friend class OrderBookMapBase<OrderBookBoostFlatMap>;
    boost::container::flat_map<int64_t, int64_t, std::greater<>> m_bids;
    boost::container::flat_map<int64_t, int64_t>                 m_asks;
};

class OrderBookSTDFlatMap : public OrderBookMapBase<OrderBookSTDFlatMap> {
    friend class OrderBookMapBase<OrderBookSTDFlatMap>;
    std::flat_map<int64_t, int64_t, std::greater<>> m_bids;
    std::flat_map<int64_t, int64_t>                 m_asks;
};

// ── Aggressive: flat hash map + 3-level bitset ─────────────────────────────

class OrderBookAggressive : public OrderBookBase<OrderBookAggressive> {
public:
    OrderBookAggressive(const OrderBookAggressive&)            = delete;
    OrderBookAggressive& operator=(const OrderBookAggressive&) = delete;

    inline OrderBookAggressive() {
        m_map = static_cast<Slot*>(std::aligned_alloc(64, sizeof(Slot) * MAP_CAP));
        for (int i = 0; i < MAP_CAP; ++i) m_map[i].id = EMPTY_ID;
        for (int s = 0; s < 2; ++s)
            m_qty[s] = new int64_t[MAX_PRICE + 1]();
        std::memset(m_l1, 0, sizeof(m_l1));
        std::memset(m_l2, 0, sizeof(m_l2));
        std::memset(m_l3, 0, sizeof(m_l3));
    }

    inline ~OrderBookAggressive() {
        std::free(m_map);
        for (int s = 0; s < 2; ++s) delete[] m_qty[s];
    }

    inline void add_order(uint64_t id, int side,
                          int64_t price, int64_t quantity) noexcept {
        map_insert(id, (uint32_t)price, (uint16_t)quantity, (uint8_t)side);
        int64_t& q = m_qty[side][price];
        if (q == 0) [[unlikely]] mark_price(side, (int)price);
        q += quantity;
    }

    inline void cancel_order(uint64_t id) noexcept {
        int idx = map_find_idx(id);
        if (idx < 0) [[unlikely]] return;
        const int      side  = m_map[idx].side;
        const int      price = (int)m_map[idx].price;
        const uint16_t qty   = m_map[idx].qty;
        map_erase_at(idx);
        int64_t& q = m_qty[side][price];
        q -= qty;
        if (q == 0) [[unlikely]] unmark_price(side, price);
    }

    inline int64_t best_bid() const noexcept { return find_max(0); }
    inline int64_t best_ask() const noexcept { return find_min(1); }

private:
    // ── open-addressing hash map ──────────────────────────────────────────
    static constexpr int      MAP_LOG2 = 21;               // 2^21 = 2 097 152 slots
    static constexpr int      MAP_CAP  = 1 << MAP_LOG2;
    static constexpr int      MAP_MASK = MAP_CAP - 1;
    static constexpr uint64_t EMPTY_ID = 0;

    struct Slot {                                           // 16 bytes, 4 per cache line
        uint64_t id;
        uint32_t price;
        uint16_t qty;
        uint8_t  side;
        uint8_t  _pad;
    };
    static_assert(sizeof(Slot) == 16);

    Slot*    m_map;                                         // 32 MB heap, 64-byte aligned
    int64_t* m_qty[2];                                      // m_qty[side][price]

    static constexpr int MAX_PRICE = 1'000'000;
    static constexpr int L1_WORDS  = (MAX_PRICE >> 6) + 1;           // 15 626
    static constexpr int L2_WORDS  = ((L1_WORDS - 1) >> 6) + 1;     // 245
    static constexpr int L3_WORDS  = ((L2_WORDS - 1) >> 6) + 1;     // 4

    uint64_t m_l1[2][L1_WORDS];
    uint64_t m_l2[2][L2_WORDS];
    uint64_t m_l3[2][L3_WORDS];

    inline int map_bucket(uint64_t key) const noexcept {
        return (int)((key * 11400714819323198485ULL) >> (64 - MAP_LOG2));
    }

    inline int map_find_idx(uint64_t id) const noexcept {
        for (int i = map_bucket(id); ; i = (i + 1) & MAP_MASK) {
            if (m_map[i].id == id)       return i;
            if (m_map[i].id == EMPTY_ID) return -1;
        }
    }

    inline void map_insert(uint64_t id, uint32_t price,
                           uint16_t qty, uint8_t side) noexcept {
        for (int i = map_bucket(id); ; i = (i + 1) & MAP_MASK) {
            if (m_map[i].id == EMPTY_ID) {
                m_map[i] = {id, price, qty, side, 0};
                return;
            }
        }
    }

    // Backward-shift deletion: no tombstones → probe chains stay short forever.
    inline void map_erase_at(int hole) noexcept {
        int j = (hole + 1) & MAP_MASK;
        while (m_map[j].id != EMPTY_ID) {
            int nat = map_bucket(m_map[j].id);
            bool can_fill = (hole < j) ? (nat <= hole || nat > j)
                                       : (nat <= hole && nat > j);
            if (can_fill) { m_map[hole] = m_map[j]; hole = j; }
            j = (j + 1) & MAP_MASK;
        }
        m_map[hole].id = EMPTY_ID;
    }

    inline void mark_price(int side, int price) noexcept {
        const int w1 = price >> 6, b1 = price & 63;
        const int w2 = w1    >> 6, b2 = w1    & 63;
        const int w3 = w2    >> 6, b3 = w2    & 63;
        m_l1[side][w1] |= (1ULL << b1);
        m_l2[side][w2] |= (1ULL << b2);
        m_l3[side][w3] |= (1ULL << b3);
    }

    inline void unmark_price(int side, int price) noexcept {
        const int w1 = price >> 6, b1 = price & 63;
        m_l1[side][w1] &= ~(1ULL << b1);
        if (m_l1[side][w1]) return;
        const int w2 = w1 >> 6, b2 = w1 & 63;
        m_l2[side][w2] &= ~(1ULL << b2);
        if (m_l2[side][w2]) return;
        const int w3 = w2 >> 6, b3 = w2 & 63;
        m_l3[side][w3] &= ~(1ULL << b3);
    }

    // Highest set price (best bid): L3 → L2 → L1 via clz
    inline int64_t find_max(int side) const noexcept {
        for (int i3 = L3_WORDS - 1; i3 >= 0; --i3) {
            uint64_t v3 = m_l3[side][i3];
            if (!v3) continue;
            int w2    = i3 * 64 + (63 - __builtin_clzll(v3));
            int w1    = w2 * 64 + (63 - __builtin_clzll(m_l2[side][w2]));
            int price = w1 * 64 + (63 - __builtin_clzll(m_l1[side][w1]));
            return price;
        }
        return 0;
    }

    // Lowest set price (best ask): L3 → L2 → L1 via ctz
    inline int64_t find_min(int side) const noexcept {
        for (int i3 = 0; i3 < L3_WORDS; ++i3) {
            uint64_t v3 = m_l3[side][i3];
            if (!v3) continue;
            int w2    = i3 * 64 + __builtin_ctzll(v3);
            int w1    = w2 * 64 + __builtin_ctzll(m_l2[side][w2]);
            int price = w1 * 64 + __builtin_ctzll(m_l1[side][w1]);
            return price;
        }
        return 0;
    }
};
