#include <iostream>
#include <mockdatafeed.hpp>
#include <marketlevel2data.hpp>
#include <orderbook.hpp>
#include <fstream>
#include <boost/chrono.hpp>

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
    std::ofstream file;
    file.open("benchmark.csv");
    file << "iterations,period,time,processCalls,getBestAskAndBidCalls" << std::endl;
    for (size_t pow = 3; pow < 7; ++pow) {
        size_t i_max = std::pow(10, pow);
        for (size_t pow_period = 1; pow_period < 4; ++pow_period) {
            size_t period = std::pow(10,pow_period);
            OrderBook book;
            size_t processCalls{0}, getBestAskAndBidCalls{0};
            auto t1 = boost::chrono::high_resolution_clock::now();
            for (size_t i = 0; i < i_max; ++i) {
                std::istringstream in_l{data_set[i]};
                auto md{MarketData::fromStr(in_l)};
                book.processOrder(md);
                processCalls++;
                if((i+1)%period == 0){
                    for (const auto& t : ticker_set) {
                        auto best_prices = book.getBestAskAndBid(t);
                        getBestAskAndBidCalls++;
                    }
                }
            }
            auto t2 = boost::chrono::high_resolution_clock::now();
            auto execution_time = (boost::chrono::duration_cast<boost::chrono::milliseconds>(t2 - t1));
            std::cout << "iterations: " << i_max << "; testing period: "<< period << "; time: " << execution_time << "\n";
            file << i_max << "," << period << "," << execution_time << "," << processCalls << "," << getBestAskAndBidCalls << std::endl;
        }
    }
    /* based on the execution times from the above code, it results that getBestAskAndBid takes microseconds to run.
     * here I do implement a test run in which the getBestAskAndBid is called every second based on system clock.
     * One million Orders are feeded into the system in the shortest time possible. Single thread serial execution makes
     * data race impossible. In a real system an asincronous io and multi-thread approach and inter-process communication should be used, jointly with a queue
     * system that stores the orders that might arrive while getBestAskAndBid is in execution.
     */
    {
        std::cout << "Starting the test with timed .getBestAskAndBid()...\n";
        OrderBook book;
        std::vector<boost::tuple<double, double>> best_prices(ticker_set.size());
        auto t1 = boost::chrono::high_resolution_clock::now();
        for (auto in_l: data_set) {
            if (boost::chrono::duration_cast<boost::chrono::milliseconds>
                        (boost::chrono::system_clock::now().time_since_epoch()).count() % 1000 == 0) { // every 1 second
                auto tik_idx{0};
                for (const auto &t: ticker_set) {
                    best_prices[tik_idx] = book.getBestAskAndBid(t);
                    tik_idx++;
                }
            }
            std::istringstream in_s{in_l};
            auto md{MarketData::fromStr(in_s)};
            book.processOrder(md);
        }
        auto t2 = boost::chrono::high_resolution_clock::now();
        auto execution_time = (boost::chrono::duration_cast<boost::chrono::milliseconds>(t2 - t1));
        std::cout << "execution time for one call every second: " << execution_time << "\n";
    }
    file.close();
    return 0;
}