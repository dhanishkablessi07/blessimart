#pragma once
#include <drogon/HttpController.h>

// Buyer ratings + comments per product.
class ReviewController : public drogon::HttpController<ReviewController>
{
  public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(ReviewController::list, "/api/products/{1}/reviews", drogon::Get);
    ADD_METHOD_TO(ReviewController::add, "/api/products/{1}/reviews", drogon::Post);
    METHOD_LIST_END

    void list(const drogon::HttpRequestPtr &req,
              std::function<void(const drogon::HttpResponsePtr &)> &&callback,
              int productId);
    void add(const drogon::HttpRequestPtr &req,
             std::function<void(const drogon::HttpResponsePtr &)> &&callback,
             int productId);
};
