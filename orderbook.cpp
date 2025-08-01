#include "orderbook.h"
#include <sstream>
#include <iomanip>
#include <algorithm>

OrderBook::OrderBook() {
    for (int i = 0; i < MAX_LEVELS; ++i) {
        topBids[i] = {0.0, 0, 0};
        topAsks[i] = {0.0, 0, 0};
    }
}

std::string OrderBook::formatPrice(double price) const {
    if (price == 0) return "";
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);
    oss << price;
    std::string result = oss.str();

    // remove trailing zeros after decimal point
    while (result.back() == '0' && result[result.size()-2] != '.') {
        result.pop_back();
    }

    return result;
}

std::string OrderBook::formatState(const std::string& tsRecv, const std::string& tsEvent,
                                const std::string& action, const std::string& side,
                                const std::string& price, const std::string& size,
                                const std::string& flags, const std::string& ts_in_delta,
                                const std::string& sequence, const std::string& orderId,
                                const std::string& symbol, int depth) const {
    std::ostringstream oss;
    
    oss << tsRecv << ","
        << tsEvent << ","
        << "10,2,1108," // rtype,publisher_id,instrument_id
        << action << ","
        << side << ","
        << depth << ",";

    // format price and size fields
    if (action == "R") {
        oss << ",0,";
    } else {
        oss << formatPrice(std::stod(price)) << "," << size << ",";
    }

    oss << flags << ","
        << ts_in_delta << ","
        << sequence << ",";

    // format orderbook state
    std::vector<PriceLevel> orderedBids;
    std::vector<PriceLevel> orderedAsks;
    
    for (const auto& bid : bids) {
        orderedBids.push_back(bid.second);
        if (orderedBids.size() >= MAX_LEVELS) break;
    }
    
    for (const auto& ask : asks) {
        orderedAsks.push_back(ask.second);
        if (orderedAsks.size() >= MAX_LEVELS) break;
    }
    
    // fill in empty levels if needed
    while (orderedBids.size() < MAX_LEVELS) {
        orderedBids.push_back({0.0, 0, 0});
    }
    while (orderedAsks.size() < MAX_LEVELS) {
        orderedAsks.push_back({0.0, 0, 0});
    }
    
    // output in interleaved format
    for (size_t i = 0; i < MAX_LEVELS; ++i) {
        if (i > 0) oss << ",";
        
        const auto& bid = orderedBids[i];
        const auto& ask = orderedAsks[i];
        
        oss << formatPrice(bid.price) << ","
            << (bid.totalSize > 0 ? std::to_string(bid.totalSize) : "0") << ","
            << (bid.count > 0 ? std::to_string(bid.count) : "0") << ","
            << formatPrice(ask.price) << ","
            << (ask.totalSize > 0 ? std::to_string(ask.totalSize) : "0") << ","
            << (ask.count > 0 ? std::to_string(ask.count) : "0");
    }
    
    // symbol and order ID
    oss << "," << symbol << "," << orderId;
    return oss.str();
}

int OrderBook::getPriceLevel(char side, double price) const {
    int level = 0;
    if (side == 'B') {
        // for bids iterate in descending price order
        for (const auto& bid : bids) {
            if (bid.first <= price) {
                return level;
            }
            level++;
        }
    } else {
        // for asks iterate in ascending price order
        for (const auto& ask : asks) {
            if (ask.first >= price) {
                return level;
            }
            level++;
        }
    }

    return level;
}

void OrderBook::updatePriceLevel(char side, double price, int sizeChange, bool isRemoval) {
    if (side == 'B') {
        auto it = bids.find(price);
        if (isRemoval) {
            if (it != bids.end()) {
                it->second.totalSize += sizeChange;
                it->second.count--;
                if (it->second.count == 0) {
                    bids.erase(it);
                }
            }
        } else {
            if (it == bids.end()) {
                bids[price] = {price, sizeChange, 1};
            } else {
                it->second.totalSize += sizeChange;
                it->second.count++;
            }
        }
    } else {
        auto it = asks.find(price);
        if (isRemoval) {
            if (it != asks.end()) {
                it->second.totalSize += sizeChange;
                it->second.count--;
                if (it->second.count == 0) {
                    asks.erase(it);
                }
            }
        } else {
            if (it == asks.end()) {
                asks[price] = {price, sizeChange, 1};
            } else {
                it->second.totalSize += sizeChange;
                it->second.count++;
            }
        }
    }
}

std::string OrderBook::processOrder(const std::string& tsRecv, const std::string& tsEvent,
                                 const std::string& action, const std::string& side,
                                 const std::string& price, const std::string& size,
                                 const std::string& orderId, const std::string& flags,
                                 const std::string& ts_in_delta, const std::string& sequence,
                                 const std::string& symbol) {
    lastSymbol = symbol;
    int depth = 0;
    
    if (action == "R") {
        return formatState(tsRecv, tsEvent, action, side, "", "0", flags, ts_in_delta, 
                         sequence, orderId, symbol);
    }

    double priceVal = price.empty() ? 0.0 : std::stod(price);
    int sizeVal = size.empty() ? 0 : std::stoi(size);

    Order order{priceVal, sizeVal, side[0], orderId};

    // don't modify the book for 'N'
    if (action == "T" && side == "N") {
        return formatState(tsRecv, tsEvent, action, side, price, size, flags, ts_in_delta,
                         sequence, orderId, symbol);
    }

    if (action == "A") {
        depth = getPriceLevel(order.side, priceVal);
        addOrder(order);
    } else if (action == "C") {
        depth = getPriceLevel(order.side, priceVal);
        removeOrder(orderId);
    } else if (action == "T" || action == "F") {
        depth = getPriceLevel(side[0], priceVal);
        
        if (action == "T") {
            tradeSequence[orderId] = {order, false};
        } else if (action == "F") {
            auto it = tradeSequence.find(orderId);
            if (it != tradeSequence.end()) {
                it->second.second = true;
            }
        }
    }
    
    updateTopLevels();
    
    return formatState(tsRecv, tsEvent, action, side, price, size, flags, ts_in_delta,
                     sequence, orderId, symbol, depth);
}

void OrderBook::addOrder(const Order& order) {
    orders[order.orderId] = order;
    updatePriceLevel(order.side, order.price, order.size);
}

void OrderBook::removeOrder(const std::string& orderId) {
    auto it = orders.find(orderId);
    if (it == orders.end()) return;

    const Order& order = it->second;
    updatePriceLevel(order.side, order.price, -order.size, true);
    orders.erase(it);
}

void OrderBook::updateTopLevels() {
    int bidIdx = 0;
    for (const auto& bid : bids) {
        if (bidIdx >= MAX_LEVELS) break;
        topBids[bidIdx++] = {bid.second.price, bid.second.totalSize, bid.second.count};
    }
    while (bidIdx < MAX_LEVELS) {
        topBids[bidIdx++] = {0.0, 0, 0};
    }

    int askIdx = 0;
    for (const auto& ask : asks) {
        if (askIdx >= MAX_LEVELS) break;
        topAsks[askIdx++] = {ask.second.price, ask.second.totalSize, ask.second.count};
    }
    while (askIdx < MAX_LEVELS) {
        topAsks[askIdx++] = {0.0, 0, 0};
    }
}
