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
//Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.#ifndef JIT_MARKETLEVEL2DATA_HPP

#include <string>
#include <boost/make_shared.hpp>
#pragma once


class MarketData {
public:
    enum class Action{add, update, cancel};
    enum class Side{ask, bid};
    /**
     * MarketDataObjest: the role of this object is to parse the order string and setup marked data which
     * will be consumed by the OrderBook. All the numerical precision tests shall be done here.
     * @param in input stream
     * @return a shared poionter to MarketData
     */
    static boost::shared_ptr<MarketData> fromStr(std::istringstream & in){

        auto result = boost::make_shared<MarketData>(MarketData());
        enum position{start=0, timestamp=0, orderid=1, action=2, other=3, ticker=3, side=4, price=5, size=6, end=7};
        auto p_i = static_cast<size_t>(position::start);
        std::string token;
        while (std::getline(in,token,'|')){
//            std::cout << token << "\n";
            switch (p_i) {
                case position::timestamp:
                    result->setTimestamp(stoull(token));
                    break;
                case position::orderid:
                    result->setOrderId(token);
                    break;
                case position::action:
                    if(token[0]=='c'){
                        result->setAction(Action::cancel);
                        p_i = position::size; // cancel will ignore any other field
                    }
                    if(token[0]=='u'){
                        result->setAction(Action::update);
                        p_i = position::price; // can only update the size
                    }
                    if(token[0]=='a')result->setAction(Action::add);
                    break;
                case position::ticker:
                    result->setTicker(token);
                    break;
                case position::side:
                    result->setSide(token[0]=='S'?Side::ask:Side::bid); // this assumes any char but 'S' are good for bids
                    break;
                case position::price:
                    result->setPrice(std::stod(token));
                    break;
                case position::size:
                    result->processable_= result->setSize(std::stoul(token));
                    break;
                default:
                    result->processable_=false;
            }
            p_i++;
        }
        return result;
    }

    [[nodiscard]] uint64_t getTimestamp() const {
        return timestamp_;
    }

    [[nodiscard]] const std::string &getOrderId() const {
        return order_id_;
    }

    [[nodiscard]] Action getAction() const {
        return action_;
    }

    [[nodiscard]] const std::string &getTicker() const {
        return ticker_;
    }

    [[nodiscard]] Side getSide() const {
        return side_;
    }

    [[nodiscard]] double getPrice() const {
        return price_;
    }

    [[nodiscard]] std::uint32_t getSize() const {
        return size_;
    }

    [[nodiscard]] bool isProcessable() const { return processable_; }

private:
    MarketData() = default;
    std::uint64_t timestamp_;
    std::string order_id_;
    Action action_;
    std::string ticker_;
    Side side_;
    double price_;
    std::uint32_t size_;
    bool processable_{true};

    void setTimestamp(const std::uint64_t &timestamp) {
        timestamp_ = timestamp;
    }

    void setOrderId(const std::string &orderId) {
        order_id_ = orderId;
    }

    void setAction(Action action) {
        action_ = action;
    }

    void setTicker(const std::string &ticker) {
        ticker_ = ticker;
    }

    void setSide(Side side) {
        side_ = side;
    }

    void setPrice(double price) {
        price_ = price;
    }

    bool setSize(std::uint32_t size){
        size_ = size;
        return size;
    }
};
