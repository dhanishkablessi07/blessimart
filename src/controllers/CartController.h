#pragma once
#include <drogon/HttpController.h>

// Buyer cart: list, add, update quantity, remove.
class CartController : public drogon::HttpController<CartController>
{
  public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(CartController::list, "/api/cart", drogon::Get);
    ADD_METHOD_TO(CartController::add, "/api/cart", drogon::Post);
    ADD_METHOD_TO(CartController::update, "/api/cart/{1}", drogon::Put);
    ADD_METHOD_TO(CartController::remove, "/api/cart/{1}", drogon::Delete);
    METHOD_LIST_END

    void list(const drogon::HttpRequestPtr &req,
              std::function<void(const drogon::HttpResponsePtr &)> &&callback);
    void add(const drogon::HttpRequestPtr &req,
             std::function<void(const drogon::HttpResponsePtr &)> &&callback);
    void update(const drogon::HttpRequestPtr &req,
                std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                int id);
    void remove(const drogon::HttpRequestPtr &req,
                std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                int id);
};
