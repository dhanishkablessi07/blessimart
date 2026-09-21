#pragma once
#include <drogon/HttpController.h>

// Checkout (no real payment) + buyer order history.
class OrderController : public drogon::HttpController<OrderController>
{
  public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(OrderController::checkout, "/api/checkout", drogon::Post);
    ADD_METHOD_TO(OrderController::myOrders, "/api/orders", drogon::Get);
    ADD_METHOD_TO(OrderController::orderDetail, "/api/orders/{1}", drogon::Get);
    METHOD_LIST_END

    void checkout(const drogon::HttpRequestPtr &req,
                  std::function<void(const drogon::HttpResponsePtr &)> &&callback);
    void myOrders(const drogon::HttpRequestPtr &req,
                  std::function<void(const drogon::HttpResponsePtr &)> &&callback);
    void orderDetail(const drogon::HttpRequestPtr &req,
                     std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                     int id);
};
