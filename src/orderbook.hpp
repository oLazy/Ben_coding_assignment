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
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/range/iterator_range.hpp>
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
struct tickerPriceTag{};

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
                        boost::multi_index::tag<priceTag>, BOOST_MULTI_INDEX_MEMBER(Order,double,price_)>,
                boost::multi_index::ordered_non_unique<
                        boost::multi_index::tag<tickerPriceTag>, boost::multi_index::composite_key<Order,
                                BOOST_MULTI_INDEX_MEMBER(Order,std::string,ticker),
                                BOOST_MULTI_INDEX_MEMBER(Order,double,price_)>
                                >
        >
> OrderSet;

template<typename MultiIndexContainer, typename Tag>
auto getOrdersInContainerByTag(const MultiIndexContainer& s)
{
    /* obtain a reference to the index tagged by Tag */
    const typename boost::multi_index::index<MultiIndexContainer,Tag>::type& i = boost::multi_index::get<Tag>(s);
    return std::pair{i.begin(),i.end()};
}

double getMinPriceForTickerIn(std::string const& ticker, OrderSet const& o){
    auto range = o.get<tickerPriceTag>().equal_range(ticker);
    if (boost::empty(range))return 0.;
    return(boost::begin(boost::make_iterator_range(range)))->price_;
}

double getMaxPriceForTickerIn(std::string const& ticker, OrderSet const& o){
    auto range = o.get<tickerPriceTag>().equal_range(ticker);
    if (boost::empty(range))return 0.;
    return(boost::rbegin(boost::make_iterator_range(range)))->price_;
}

struct UpdateSize{ // utility function to update unidexed field in the order_set
    explicit UpdateSize(std::uint32_t new_size):new_size_(new_size){}
    void operator()(Order& o){
        o.size_ = new_size_;
    }
private:
    std::uint32_t new_size_{0};
};

class OrderBook{
private:
    OrderSet ask, bid;

    void add(boost::shared_ptr<MarketData> const& md){ // O(log(n))
        OrderSet *target{nullptr};
        target = (md->getSide()==MarketData::Side::ask)?&ask:&bid;
        target->insert({md->getOrderId(), md->getTicker(), md->getPrice(), md->getSize()});
    };

    void update(boost::shared_ptr<MarketData> const& md){ // O(1)
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
        auto &id_index = ask.get<0>();
        auto iter = id_index.find(md->getOrderId());
        if (iter==ask.end()){ // not in ask
            auto &bid_id_index = bid.get<0>();
            iter = bid_id_index.find(md->getOrderId());
            bid.erase(iter);
            return;
        }
        ask.erase(iter);
    };

public:
    bool empty(){ return (ask.empty()&&bid.empty());}
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

    boost::tuple<double, double> getBestAskAndBid(std::string const& ticker) {
        return {getMinPriceForTickerIn(ticker, ask), getMaxPriceForTickerIn(ticker, bid)};
    }

    // some utility interface, for testing
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
    };
