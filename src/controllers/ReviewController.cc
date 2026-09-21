#include "controllers/ReviewController.h"

using namespace drogon;

static void sendError(auto &&callback, const std::string &msg,
                      drogon::HttpStatusCode code = drogon::k400BadRequest)
{
    Json::Value j;
    j["ok"] = false;
    j["message"] = msg;
    auto resp = HttpResponse::newHttpJsonResponse(j);
    resp->setStatusCode(code);
    callback(resp);
}

// GET /api/products/{id}/reviews
void ReviewController::list(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&callback,
    int productId)
{
    auto db = drogon::app().getDbClient("default");
    db->execSqlAsync(
        "SELECT r.rating, r.comment, r.created_at, u.name AS buyer_name "
        "FROM reviews r JOIN users u ON u.id = r.buyer_id "
        "WHERE r.product_id = ? ORDER BY r.id DESC",
        [callback](const drogon::orm::Result &r) {
            Json::Value arr(Json::arrayValue);
            double sum = 0;
            for (auto &row : r)
            {
                Json::Value j;
                j["rating"] = row["rating"].as<int>();
                j["comment"] = row["comment"].as<std::string>();
                j["created_at"] = row["created_at"].as<std::string>();
                j["buyer_name"] = row["buyer_name"].as<std::string>();
                sum += row["rating"].as<int>();
                arr.append(j);
            }
            Json::Value out;
            out["ok"] = true;
            out["reviews"] = arr;
            out["count"] = (int)r.size();
            out["avg"] = r.empty() ? 0 : sum / r.size();
            callback(HttpResponse::newHttpJsonResponse(out));
        },
        [callback](const drogon::orm::DrogonDbException &e) {
            sendError(callback, std::string("DB error: ") + e.base().what());
        },
        productId);
}

// POST /api/products/{id}/reviews {rating, comment} (buyer only)
void ReviewController::add(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&callback,
    int productId)
{
    auto idOpt = req->session()->getOptional<int>("user_id");
    auto roleOpt = req->session()->getOptional<std::string>("user_role");
    if (!idOpt || !roleOpt || *roleOpt != "buyer")
    {
        sendError(callback, "Buyer login required", drogon::k401Unauthorized);
        return;
    }
    auto json = req->getJsonObject();
    if (!json)
    {
        sendError(callback, "Invalid JSON body");
        return;
    }
    int rating = (*json).get("rating", 0).asInt();
    std::string comment = (*json).get("comment", "").asString();
    if (rating < 1 || rating > 5)
    {
        sendError(callback, "Rating must be 1-5");
        return;
    }

    int buyerId = *idOpt;
    auto db = drogon::app().getDbClient("default");
    db->execSqlAsync(
        "SELECT id FROM products WHERE id = ?",
        [callback, db, buyerId, productId, rating, comment](const drogon::orm::Result &r) {
            if (r.empty())
            {
                sendError(callback, "Product not found", drogon::k404NotFound);
                return;
            }
            db->execSqlAsync(
                "INSERT INTO reviews (product_id, buyer_id, rating, comment) VALUES (?, ?, ?, ?)",
                [callback](const drogon::orm::Result &) {
                    Json::Value j;
                    j["ok"] = true;
                    j["message"] = "Review added";
                    callback(HttpResponse::newHttpJsonResponse(j));
                },
                [callback](const drogon::orm::DrogonDbException &e) {
                    sendError(callback, std::string("DB error: ") + e.base().what());
                },
                productId, buyerId, rating, comment);
        },
        [callback](const drogon::orm::DrogonDbException &e) {
            sendError(callback, std::string("DB error: ") + e.base().what());
        },
        productId);
}
