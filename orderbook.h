#ifndef ORDERBOOK_H
#define ORDERBOOK_H

#include <map>
#include <unordered_map>
#include <string>
#include <vector>
#include <array>
#include <chrono>

struct Order {
    double price;
    int size;
    char side;
    std::string orderId;
};

struct PriceLevel {
    double price;
    int totalSize;
    int count;
};

class OrderBook {
public:
    OrderBook();
    std::string processOrder(const std::string& tsRecv, const std::string& tsEvent, 
                     const std::string& action, const std::string& side,
                     const std::string& price, const std::string& size,
                     const std::string& orderId, const std::string& flags,
                     const std::string& ts_in_delta, const std::string& sequence,
                     const std::string& symbol);

protected:
    void updatePriceLevel(char side, double price, int sizeChange, bool isRemoval = false);
    std::string formatPrice(double price) const;
    std::string formatState(const std::string& tsRecv, const std::string& tsEvent,
                          const std::string& action, const std::string& side,
                          const std::string& price, const std::string& size,
                          const std::string& flags, const std::string& ts_in_delta,
                          const std::string& sequence, const std::string& orderId,
                          const std::string& symbol, int depth = 0) const;
    int getPriceLevel(char side, double price) const;
    const std::map<double, PriceLevel, std::greater<double>>& getBids() const { return bids; }
    const std::map<double, PriceLevel>& getAsks() const { return asks; }

private:
    static constexpr int MAX_LEVELS = 10;
    std::unordered_map<std::string, Order> orders;
    std::map<double, PriceLevel, std::greater<double>> bids;
    std::map<double, PriceLevel> asks;
    std::array<PriceLevel, MAX_LEVELS> topBids;
    std::array<PriceLevel, MAX_LEVELS> topAsks;
    std::string lastSymbol;
    std::unordered_map<std::string, std::pair<Order, bool>> tradeSequence; // Tracks T->F->C sequences

    void updateTopLevels();
    void addOrder(const Order& order);
    void removeOrder(const std::string& orderId);
    void processTradeSequence(const Order& tradeOrder);
};

#endif
