#pragma once
#include <drogon/HttpController.h>

// Admin: view users, view all orders, delete any product.
class AdminController : public drogon::HttpController<AdminController>
{
  public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(AdminController::users, "/api/admin/users", drogon::Get);
    ADD_METHOD_TO(AdminController::orders, "/api/admin/orders", drogon::Get);
    ADD_METHOD_TO(AdminController::deleteProduct, "/api/admin/products/{1}", drogon::Delete);
    METHOD_LIST_END

    void users(const drogon::HttpRequestPtr &req,
               std::function<void(const drogon::HttpResponsePtr &)> &&callback);
    void orders(const drogon::HttpRequestPtr &req,
                std::function<void(const drogon::HttpResponsePtr &)> &&callback);
    void deleteProduct(const drogon::HttpRequestPtr &req,
                       std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                       int productId);
};
