#include <iostream>
#include <mockdatafeed.hpp>
#include <marketlevel2data.hpp>
#include <orderbook.hpp>
#include <fstream>

int main() {
    std::cout << "beginning order set and ticker set generation ...\n";
    std::vector<std::string> data_set;
    MockDataFeed feed;
    for (size_t i = 0; i < 1000000; ++i) {
        data_set.push_back(feed.generateData());
    }
    std::vector<std::string> ticker_set;
    for (size_t i = 0; i < 2025; ++i){
        ticker_set.push_back(std::to_string(i));
    }
    std::cout << "done.\n";
    ImprovedOrderBook book;
    std::ofstream file;
    file.open("benchmark.csv");
    file << "iterations,period,time" << std::endl;
    for (size_t pow = 3; pow < 7; ++pow) {
        size_t i_max = std::pow(10, pow);
        for (size_t pow_period = 1; pow_period < 4; ++pow_period) {
            size_t period = std::pow(10,pow_period);
            auto t1 = boost::chrono::high_resolution_clock::now();
            for (size_t i = 0; i < i_max; ++i) {
                std::istringstream in_l{data_set[i]};
                auto md{MarketData::fromStr(in_l)};
                book.processOrder(md);
                if((i+1)%period == 0){
                    for (auto t : ticker_set) {
                        auto best_prices = book.getBestAskAndBid(t);
                    }
                }
            }
            auto t2 = boost::chrono::high_resolution_clock::now();
            auto execution_time = (boost::chrono::duration_cast<boost::chrono::milliseconds>(t2 - t1));
            std::cout << "iterations: " << i_max << "; testing period: "<< period << "; time: " << execution_time << "\n";
            file << i_max << "," << period << "," << execution_time << std::endl;
        }
    }
}