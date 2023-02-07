//one line to give the program's name and an idea of what it does.
//Copyright (C) 2023,  Eric Mandolesi

//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either version 2
//of the License, or (at your option) any later version.

//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.

//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.#ifndef JIT_MOCKDATAFEED_HPP


#include <marketlevel2data.hpp>
#include <orderbook.hpp>
#include <random>
#include <boost/chrono.hpp>

#pragma once

std::random_device rand_dev;
std::mt19937_64 generator(rand_dev());
int ids{0};

class MockDataFeed{
public:
    MockDataFeed() = default;
    std::string getData(){
        auto result = outcomes[index_];
        index_++;
        if(index_==outcomes.size())index_=0; // create circular buffer
        return result;
    }

private:

    size_t index_{0};
    std::string example1_1{"1568390243|abbb11|a|AAPL|B|209.00000|100"};
    std::string example1_2{"1568390244|abbb11|u|101"};
    std::string example1_3{"1568390245|abbb11|c"};
    std::string example2_1{"1568390201|abbb11|a|AAPL|B|209.00000|100"};
    std::string example2_2{"1568390202|abbb12|a|AAPL|S|210.00000|10"};
    std::string example2_3{"1568390204|abbb11|u|10"};
    std::string example2_4{"1568390203|abbb12|u|101"};
    std::string example2_5{"1568390243|abbb11|c"};

    std::vector<std::string> outcomes{example1_1, example1_2, example1_3, example2_1,
                                      example2_2, example2_3, example2_4, example2_5};
    std::vector<Order> ask_ticker_pool, bid_ticker_pool;
    size_t max_ask_pool_size{0}, max_bid_pool_size{0};
    class IDGenerator{
    public:
        int generateID(){
            int result{ids};
            ids++;
            return result;
        };
    };
public:
    size_t maxAskSize(){return max_ask_pool_size;}
    size_t maxBidSize(){return max_bid_pool_size;}
    std::string generateData(){
        std::uint64_t ms = boost::chrono::duration_cast<boost::chrono::milliseconds>
                (boost::chrono::system_clock::now().time_since_epoch()).count();
        std::uniform_int_distribution<int> ticker_gen(0, 2024);
        std::uniform_real_distribution<double> price_gen(0.00001,10000.00000);
        std::uniform_int_distribution<int> size_gen(1,10000);
        std::uniform_int_distribution<int> side_gen(0,1);
        std::poisson_distribution<int> action_gen(1);
        std::string result;
        // generate side
        auto side = side_gen(generator)?'B':'S';
        // generate action
        char action;
        switch (side) {
            case 'B':
                if(bid_ticker_pool.empty())action = 'a';
                else{
                    auto rnd = action_gen(generator);
                    if (rnd == 0) action = 'a';
                    if (rnd == 1) action = 'u';
                    if (rnd == 2) action = 'c';
                }
                break;
            case 'S':
                if(ask_ticker_pool.empty())action = 'a';
                else{
                    auto rnd = action_gen(generator);
                    if (rnd == 0) action = 'a';
                    if (rnd == 1) action = 'u';
                    if (rnd >= 2) action = 'c';
                }
                break;
        }
        // generate // retrieve order id - eventual price - eventual size
        int id;
        double price;
        std::uint32_t size;
        std::string ticker;
        Order retOrder;
        IDGenerator idg;
        switch (action) {
            case 'a':
            {id = idg.generateID();
                price = price_gen(generator);
                size = size_gen(generator);
                ticker = std::to_string(ticker_gen(generator));
                std::string trimmedPriceString = std::to_string(price).substr(0, std::to_string(price).find(".") + 6);
                Order toAdd{std::to_string(id), ticker, price, size};
                if(side=='S'){
                    ask_ticker_pool.push_back(toAdd);
                    max_ask_pool_size = std::max({max_ask_pool_size, ask_ticker_pool.size()});
                }
                else {
                    bid_ticker_pool.push_back(toAdd);
                    max_bid_pool_size = std::max({max_bid_pool_size, bid_ticker_pool.size()});
                }

                result=std::to_string(ms)+"|"+std::to_string(id)+"|a|"+ticker
                       +"|"+side+"|"+ trimmedPriceString +"|" + std::to_string(size);
                break;}
            case 'u':
                switch (side) {
                    case 'S':
                        size = size_gen(generator);
                        {std::uniform_int_distribution<int> randomPick(0,ask_ticker_pool.size()-1);
                            retOrder = ask_ticker_pool[randomPick(generator)];
                            result=std::to_string(ms)+"|"+retOrder.id+"|u|"+ std::to_string(size);
                            break;}
                    case 'B':
                        size = size_gen(generator);
                    {std::uniform_int_distribution<int> randomPick(0,bid_ticker_pool.size()-1);
                        retOrder = bid_ticker_pool[randomPick(generator)];
                        result=std::to_string(ms)+"|"+retOrder.id+"|u|"+ std::to_string(size);
                        break;}
                }
                break;
            case 'c':
                switch (side) {
                    case 'S':
                    {std::uniform_int_distribution<int> randomPick(0,ask_ticker_pool.size()-1);
                        int rn = randomPick(generator);
                        retOrder = ask_ticker_pool[rn];
                        ask_ticker_pool.erase(ask_ticker_pool.begin()+rn);
                        result=std::to_string(ms)+"|"+retOrder.id+"|c";
                        break;}
                    case 'B':
                    {std::uniform_int_distribution<int> randomPick(0,bid_ticker_pool.size()-1);
                        int rn = randomPick(generator);
                        retOrder = bid_ticker_pool[rn];
                        bid_ticker_pool.erase(bid_ticker_pool.begin()+rn);
                        result=std::to_string(ms)+"|"+retOrder.id+"|c";
                        break;}
                }
                break;
        }
        return result;
    }

};
