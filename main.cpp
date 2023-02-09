#include <iostream>
#include <mockdatafeed.hpp>
#include <marketlevel2data.hpp>
#include <orderbook.hpp>
#include <fstream>
#include <queue>
#include <boost/chrono.hpp>
#include <omp.h>

#define THREAD_NUM 4
int main() {

    std::queue<boost::shared_ptr<MarketData>> order_queue;
    bool stop{false};
    OrderBook book;
    std::string user_input;
    omp_set_num_threads(THREAD_NUM); // set number of threads in "parallel" blocks
#pragma omp parallel default(none) shared(user_input, stop, order_queue, book, std::cout, std::cin, std::cerr)
    {
        enum thread{feeder, bookkeeper, inquirer, interface};
        auto role = (thread)omp_get_thread_num();

        /*
         *  INTERFACE
         */
        if(role == interface)
        {
            std::cout << "Please input the ticker name you wish to observe. Print 'exit()' to quit." << std::endl;
            while(strcmp(user_input.c_str(), "exit()")!=0){
                std::cin >> user_input;
//                std::cout << user_input << std::endl;
            }
            if(strcmp(user_input.c_str(), "exit()")==0){
                stop = true;
                std::cout << "stopping the team" << std::endl;
#pragma omp cancel parallel
            }
        }

            /*
             * FEEDER
             */
        else if(role == feeder){
            MockDataFeed feed;
            while (!stop) { // as long as the program runs
                if (boost::chrono::duration_cast<boost::chrono::milliseconds>
                            (boost::chrono::system_clock::now().time_since_epoch()).count() % 10 ==
                    0) { // every 0.01 second
                    auto d = feed.generateData();
                    auto ss = std::istringstream{d};
                    order_queue.push(MarketData::fromStr(ss));
                    usleep(10); // this ensures 100 orders per second
                }
#pragma omp cancellation point parallel
            }
        }
        else if(role == bookkeeper){
            while (!stop){
                if(order_queue.empty()){
#pragma omp cancellation point parallel
                    continue;
                }else{
                    auto o = order_queue.front();
                    order_queue.pop();
                    book.processOrder(o);
                }
            }
        }

            /*
             * INQUIRER
             */
        else if(role == inquirer) {
            while (!stop) {
                if ( (boost::chrono::duration_cast<boost::chrono::milliseconds>
                              (boost::chrono::system_clock::now().time_since_epoch()).count() % 1000 == 0) ) { // every 1 second

                    auto values = book.getBestAskAndBid(user_input);
                    std::cerr << user_input << "A: " << values.get<0>() << "\t" << "B: " << values.get<1>()
                              << std::endl;
                    usleep(1000); // sleep for one millisecond to syncronise the computations/prints
                } else {
#pragma omp cancellation point parallel
                    continue;
                }
            }
        }
        std::cout << "[" << role << "] stopping job here.\n";
    }
    return 0;
}