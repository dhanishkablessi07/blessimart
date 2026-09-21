#pragma once
#include <drogon/HttpController.h>

// Public: list + detail. Seller: add / edit / delete own products.
class ProductController : public drogon::HttpController<ProductController>
{
  public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(ProductController::list, "/api/products", drogon::Get);
    ADD_METHOD_TO(ProductController::getOne, "/api/products/{1}", drogon::Get);
    ADD_METHOD_TO(ProductController::myProducts, "/api/seller/products", drogon::Get);
    ADD_METHOD_TO(ProductController::create, "/api/seller/products", drogon::Post);
    ADD_METHOD_TO(ProductController::update, "/api/seller/products/{1}", drogon::Put);
    ADD_METHOD_TO(ProductController::remove, "/api/seller/products/{1}", drogon::Delete);
    METHOD_LIST_END

    void list(const drogon::HttpRequestPtr &req,
              std::function<void(const drogon::HttpResponsePtr &)> &&callback);
    void getOne(const drogon::HttpRequestPtr &req,
                std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                int id);
    void myProducts(const drogon::HttpRequestPtr &req,
                    std::function<void(const drogon::HttpResponsePtr &)> &&callback);
    void create(const drogon::HttpRequestPtr &req,
                std::function<void(const drogon::HttpResponsePtr &)> &&callback);
    void update(const drogon::HttpRequestPtr &req,
                std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                int id);
    void remove(const drogon::HttpRequestPtr &req,
                std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                int id);
};
