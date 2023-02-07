//Here the OrderBook is defines and implemented.
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
//Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.#ifndef JIT_ORDERBOOK_HPP
#pragma once

#include <marketlevel2data.hpp>
#include <map>
#include <set>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <ostream>
#include <algorithm>
#include <iterator>

struct Order{
    std::string id;
    std::string ticker;
    double price_{0.};
    std::uint32_t size_{0};

    Order() = default;
    Order(const std::string &id, const std::string &ticker, double price, uint32_t size) : id(id), ticker(ticker),
                                                                                           price_(price), size_(size) {}

    friend std::ostream &operator<<(std::ostream &os, const Order &order) {
        os << "id: " << order.id << " ticker: " << order.ticker << " price_: " << order.price_ << " size_: "
           << order.size_;
        return os;
    }
};

/* tags for accessing the corresponding indices of OrderSet */

struct idTag{};
struct tickerTag{};
struct priceTag{};

/* Define a multi_index_container of Order with following indices:
 *   - a unique index sorted by Order::id,
 *   - a non-unique index sorted by Order::ticker,
 *   - a non-unique index sorted by Order::price_.
 *   size_ doesn't need to be indexed, as is at most removed/updated via Order::id handler
 */


typedef boost::multi_index::multi_index_container<
        Order,
        boost::multi_index::indexed_by<
                boost::multi_index::ordered_unique<
                        boost::multi_index::tag<idTag>, BOOST_MULTI_INDEX_MEMBER(Order,std::string,id)>,
                boost::multi_index::ordered_non_unique<
                        boost::multi_index::tag<tickerTag>, BOOST_MULTI_INDEX_MEMBER(Order,std::string,ticker)>,
                boost::multi_index::ordered_non_unique<
                        boost::multi_index::tag<priceTag>, BOOST_MULTI_INDEX_MEMBER(Order,double,price_)>
        >
> OrderSet;

template<typename MultiIndexContainer, typename Tag>
auto getOrdersInContainerByTag(const MultiIndexContainer& s)
{
    /* obtain a reference to the index tagged by Tag */
    const typename boost::multi_index::index<MultiIndexContainer,Tag>::type& i = boost::multi_index::get<Tag>(s);
    return std::pair{i.begin(),i.end()};
}

struct UpdateSize{
    explicit UpdateSize(std::uint32_t new_size):new_size_(new_size){}
    void operator()(Order& o){
        o.size_ = new_size_;
    }
private:
    std::uint32_t new_size_{0};
};

class OrderBook{
    typedef std::string ticker;
    typedef std::string orederId;
public:
    void processOrder(boost::shared_ptr<MarketData> const& md){
        if(!md->isProcessable())return; // discard corrupted order
        switch (md->getAction()) {
            case MarketData::Action::add:
                add(md);
                break;
            case MarketData::Action::update:
                update(md);
                break;
            case MarketData::Action::cancel:
                cancel(md);
                break;
        }
    }
private:
    std::map<orederId , MarketData::Side> p_idAskBid;
    std::unordered_map<orederId ,Order> bid, ask;

    void add(boost::shared_ptr<MarketData> const& md){ // adding order assumes an orderId not in book yet
        if(md->getSide()==MarketData::Side::bid){
            p_idAskBid[md->getOrderId()] = MarketData::Side::bid;
            bid[md->getOrderId()] = Order(md->getOrderId(), md->getTicker(), md->getPrice(), md->getSize());
        }
        else {
            p_idAskBid[md->getOrderId()] = MarketData::Side::ask;
            ask[md->getOrderId()] = {md->getOrderId(), md->getTicker(), md->getPrice(), md->getSize()};

        }
    }
    void update(boost::shared_ptr<MarketData> const& md){
        if(p_idAskBid[md->getOrderId()] == MarketData::Side::bid) {
            bid[md->getOrderId()].size_ = md->getSize();
        }
        else {
            ask[md->getOrderId()].size_ = md->getSize();
        }
    }
    void cancel(boost::shared_ptr<MarketData> const& md) {
        if (p_idAskBid[md->getOrderId()] == MarketData::Side::bid) {
            bid.erase(md->getOrderId());
        } else {
            ask.erase(md->getOrderId());
        }
        p_idAskBid.erase(md->getOrderId());
    }
public:
    double getPriceFor(std::string const& id){
        if(p_idAskBid.find(id) == p_idAskBid.end())return 0;
        if (p_idAskBid.at(id) == MarketData::Side::bid) return bid.at(id).price_;
        else return ask.at(id).price_;}
    uint32_t getSizeFor(std::string const& id) const{
        if(p_idAskBid.find(id) == p_idAskBid.end())return 0;
        if (p_idAskBid.at(id) == MarketData::Side::bid) return bid.at(id).size_;
        else return ask.at(id).size_;
    }
};

class ImprovedOrderBook{
private:
    OrderSet ask, bid;
    void add(boost::shared_ptr<MarketData> const& md){ // O(log(n))
        OrderSet *target{nullptr};
        target = (md->getSide()==MarketData::Side::ask)?&ask:&bid;
        target->insert({md->getOrderId(), md->getTicker(), md->getPrice(), md->getSize()});
    };
    void update(boost::shared_ptr<MarketData> const& md){ // O(1)
        OrderSet *target{nullptr};
        target = &ask;
        auto &id_index = ask.get<0>();
        auto iter = id_index.find(md->getOrderId());
        if(iter==ask.end()){ // not in ask!;
            auto &bid_id_index = bid.get<0>();
            iter = bid_id_index.find(md->getOrderId());
            bid.modify(iter, UpdateSize(md->getSize()));
            return;
        }
        ask.modify(iter, UpdateSize(md->getSize()));
    };
    void cancel(boost::shared_ptr<MarketData> const& md){ // O(1)
        OrderSet *target{nullptr};
        target = (md->getSide()==MarketData::Side::ask)?&ask:&bid;
        auto &id_index = target->get<0>();
        auto iter = id_index.find(md->getOrderId());
        target->erase(iter);
    };

public:

    void processOrder(boost::shared_ptr<MarketData> const& md){
        if(!md->isProcessable())return; // discard corrupted order
        switch (md->getAction()) {
            case MarketData::Action::add:
                add(md);
                break;
            case MarketData::Action::update:
                update(md);
                break;
            case MarketData::Action::cancel:
                cancel(md);
                break;
        }
    }
    double getPriceFor(std::string const& id) {
        OrderSet *target = &ask;
        auto &id_index = target->get<0>();
        auto iter = id_index.find(id);
        if (iter == target->end()) { // not in ask!
            target = &bid;
        }
        return getOrdersInContainerByTag<OrderSet,idTag>(*target).first->price_;
    }
    std::uint32_t getSizeFor(std::string const& id){
        OrderSet *target = &ask;
        auto &id_index = target->get<0>();
        auto iter = id_index.find(id);
        if (iter == target->end()) { // not in ask!
            target = &bid;
        }
        return getOrdersInContainerByTag<OrderSet,idTag>(*target).first->size_;
    }
//    boost::tuple<double, double> getBestBidAndAsk(std::string const& ticker){
//        return {0,0};
//    }
};