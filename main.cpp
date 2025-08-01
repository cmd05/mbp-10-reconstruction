#include "orderbook.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <chrono>

std::vector<std::string> splitCSV(const std::string& line) {
    std::vector<std::string> result;
    std::stringstream ss(line);
    std::string item;
    
    while (std::getline(ss, item, ','))
        result.push_back(item);

    return result;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <mbo_csv_file>" << "\r\n";
        return 1;
    }

    std::ifstream inFile(argv[1]);
    if (!inFile.is_open()) {
        std::cerr << "Error: Could not open file " << argv[1] << "\r\n";
        return 1;
    }

    std::ofstream outFile("mbp_out.csv", std::ios::binary);
    if (!outFile.is_open()) {
        std::cerr << "Error: Could not open output file mbp_out.csv" << "\r\n";
        return 1;
    }

    // Print header
    outFile << ",ts_recv,ts_event,rtype,publisher_id,instrument_id,action,side,depth,price,size,"
            << "flags,ts_in_delta,sequence,bid_px_00,bid_sz_00,bid_ct_00,ask_px_00,ask_sz_00,"
            << "ask_ct_00,bid_px_01,bid_sz_01,bid_ct_01,ask_px_01,ask_sz_01,ask_ct_01,"
            << "bid_px_02,bid_sz_02,bid_ct_02,ask_px_02,ask_sz_02,ask_ct_02,bid_px_03,"
            << "bid_sz_03,bid_ct_03,ask_px_03,ask_sz_03,ask_ct_03,bid_px_04,bid_sz_04,"
            << "bid_ct_04,ask_px_04,ask_sz_04,ask_ct_04,bid_px_05,bid_sz_05,bid_ct_05,"
            << "ask_px_05,ask_sz_05,ask_ct_05,bid_px_06,bid_sz_06,bid_ct_06,ask_px_06,"
            << "ask_sz_06,ask_ct_06,bid_px_07,bid_sz_07,bid_ct_07,ask_px_07,ask_sz_07,"
            << "ask_ct_07,bid_px_08,bid_sz_08,bid_ct_08,ask_px_08,ask_sz_08,ask_ct_08,"
            << "bid_px_09,bid_sz_09,bid_ct_09,ask_px_09,ask_sz_09,ask_ct_09,symbol,order_id"
            << "\r\n";

    std::string line;
    std::getline(inFile, line); // Skip header

    // std::ifstream sampleFile("mbp.csv");
    // std::getline(sampleFile, line);

    OrderBook orderbook;
    int lineCount = 0;

    while (std::getline(inFile, line)) {
        auto fields = splitCSV(line);
        
        if (fields.size() < 15) {
            continue; // Skip invalid lines
        }

        // // Use timestamps from the MBP for testing
        // std::getline(sampleFile, line);
        // std::string ts_recv = sampleFields.size() > 1 ? sampleFields[1] : "";
        // std::string ts_event = sampleFields.size() > 2 ? sampleFields[2] : "";

        // Extract order data
        std::string ts_recv = fields.size() > 0 ? fields[0] : "";
        std::string ts_event = fields.size() > 1 ? fields[1] : "";

        std::string action = fields.size() > 5 ? fields[5] : "";
        std::string side = fields.size() > 6 ? fields[6] : "";
        std::string price = fields.size() > 7 ? fields[7] : "";
        std::string size = fields.size() > 8 ? fields[8] : "";
        std::string orderId = fields.size() > 10 ? fields[10] : "";
        std::string flags = fields.size() > 11 ? fields[11] : "";
        std::string ts_in_delta = fields.size() > 12 ? fields[12] : "";
        std::string sequence = fields.size() > 13 ? fields[13] : "";
        std::string symbol = fields.size() > 14 ? fields[14] : "";

        outFile << lineCount++ << ","
                 << orderbook.processOrder(
                    ts_recv,
                    ts_event,
                    action,
                    side,
                    price,
                    size,
                    orderId,
                    flags,
                    ts_in_delta,
                    sequence,
                    symbol
                 ) << "\r\n";
    }

    return 0;
}
