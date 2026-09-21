#include "controllers/SellerController.h"

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

static int getSellerId(const drogon::HttpRequestPtr &req)
{
    auto idOpt = req->session()->getOptional<int>("user_id");
    auto roleOpt = req->session()->getOptional<std::string>("user_role");
    if (!idOpt || !roleOpt || *roleOpt != "seller")
        return -1;
    return *idOpt;
}

// GET /api/seller/orders -- flat list of my sold items.
void SellerController::receivedOrders(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&callback)
{
    int sellerId = getSellerId(req);
    if (sellerId < 0)
    {
        sendError(callback, "Seller login required", drogon::k401Unauthorized);
        return;
    }
    auto db = drogon::app().getDbClient("default");
    db->execSqlAsync(
        "SELECT o.id AS order_id, o.status, o.address, o.created_at, "
        "u.name AS buyer_name, p.title, oi.quantity, oi.price_at_buy "
        "FROM order_items oi "
        "JOIN orders o ON o.id = oi.order_id "
        "JOIN products p ON p.id = oi.product_id "
        "JOIN users u ON u.id = o.buyer_id "
        "WHERE oi.seller_id = ? ORDER BY o.id DESC",
        [callback](const drogon::orm::Result &r) {
            Json::Value arr(Json::arrayValue);
            for (auto &row : r)
            {
                Json::Value j;
                j["order_id"] = row["order_id"].as<int>();
                j["status"] = row["status"].as<std::string>();
                j["address"] = row["address"].as<std::string>();
                j["created_at"] = row["created_at"].as<std::string>();
                j["buyer_name"] = row["buyer_name"].as<std::string>();
                j["title"] = row["title"].as<std::string>();
                j["quantity"] = row["quantity"].as<int>();
                j["price_at_buy"] = row["price_at_buy"].as<double>();
                arr.append(j);
            }
            Json::Value out;
            out["ok"] = true;
            out["sales"] = arr;
            callback(HttpResponse::newHttpJsonResponse(out));
        },
        [callback](const drogon::orm::DrogonDbException &e) {
            sendError(callback, std::string("DB error: ") + e.base().what());
        },
        sellerId);
}

// PUT /api/seller/orders/{orderId} {status}
void SellerController::updateStatus(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&callback,
    int orderId)
{
    int sellerId = getSellerId(req);
    if (sellerId < 0)
    {
        sendError(callback, "Seller login required", drogon::k401Unauthorized);
        return;
    }
    auto json = req->getJsonObject();
    if (!json)
    {
        sendError(callback, "Invalid JSON body");
        return;
    }
    std::string status = (*json).get("status", "").asString();
    if (status != "placed" && status != "shipped" &&
        status != "delivered" && status != "cancelled")
    {
        sendError(callback, "Invalid status (placed/shipped/delivered/cancelled)");
        return;
    }

    auto db = drogon::app().getDbClient("default");
    // Only allow if this seller has at least one item in the order.
    db->execSqlAsync(
        "SELECT id FROM order_items WHERE order_id = ? AND seller_id = ? LIMIT 1",
        [callback, db, orderId, status](const drogon::orm::Result &r) {
            if (r.empty())
            {
                sendError(callback, "Order not found or not yours", drogon::k404NotFound);
                return;
            }
            db->execSqlAsync(
                "UPDATE orders SET status = ? WHERE id = ?",
                [callback](const drogon::orm::Result &) {
                    Json::Value j;
                    j["ok"] = true;
                    j["message"] = "Status updated";
                    callback(HttpResponse::newHttpJsonResponse(j));
                },
                [callback](const drogon::orm::DrogonDbException &e) {
                    sendError(callback, std::string("DB error: ") + e.base().what());
                },
                status, orderId);
        },
        [callback](const drogon::orm::DrogonDbException &e) {
            sendError(callback, std::string("DB error: ") + e.base().what());
        },
        orderId, sellerId);
}
