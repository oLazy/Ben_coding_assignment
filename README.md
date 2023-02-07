# MOP
Market Order Processer (MOP) is a dummy project. It implements the class OrderBook which acts as a market order book. It
* stores the current status for buy/sell orders including: 
    * order id
    * ticker/product code
    * bidding/asking price
    * order size
* implements a method to process new order
* implements a method to retrieve the best (lower) asking and best (higher) bid price at any given time.

The orders are read in string format and a feeder mocker has been inplemented in the test folder.
The following assumptions hold:
* the orders arrive in chronological order. It doesn't happen that at second 15 the feeder produces a new order related to second 5. This is because if best ask and bid prices are requested at second 10, the results might be wrong
* the orders make sense. No cancel order can arrive for an order id that was never added. Same holds for update orders. 

MOP is based on the boost libraries (http://boost.org) for testing, multi-indexed containers, timers and more. 