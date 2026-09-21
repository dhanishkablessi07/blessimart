#pragma once
#include <drogon/HttpController.h>

// Seller: view orders containing my products + update status.
class SellerController : public drogon::HttpController<SellerController>
{
  public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(SellerController::receivedOrders, "/api/seller/orders", drogon::Get);
    ADD_METHOD_TO(SellerController::updateStatus, "/api/seller/orders/{1}", drogon::Put);
    METHOD_LIST_END

    void receivedOrders(const drogon::HttpRequestPtr &req,
                        std::function<void(const drogon::HttpResponsePtr &)> &&callback);
    void updateStatus(const drogon::HttpRequestPtr &req,
                      std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                      int orderId);
};
