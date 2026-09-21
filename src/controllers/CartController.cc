#include "controllers/CartController.h"

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

// Returns buyer_id or -1. Only role "buyer" may use cart.
static int getBuyerId(const drogon::HttpRequestPtr &req)
{
    auto idOpt = req->session()->getOptional<int>("user_id");
    auto roleOpt = req->session()->getOptional<std::string>("user_role");
    if (!idOpt || !roleOpt || *roleOpt != "buyer")
        return -1;
    return *idOpt;
}

// GET /api/cart -> items + total
void CartController::list(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&callback)
{
    int buyerId = getBuyerId(req);
    if (buyerId < 0)
    {
        sendError(callback, "Buyer login required", drogon::k401Unauthorized);
        return;
    }
    auto db = drogon::app().getDbClient("default");
    db->execSqlAsync(
        "SELECT c.id, c.quantity, p.id AS product_id, p.title, p.price, p.stock "
        "FROM cart_items c JOIN products p ON p.id = c.product_id "
        "WHERE c.buyer_id = ? ORDER BY c.id DESC",
        [callback](const drogon::orm::Result &r) {
            Json::Value arr(Json::arrayValue);
            double total = 0.0;
            for (auto &row : r)
            {
                Json::Value j;
                j["id"] = row["id"].as<int>();
                j["quantity"] = row["quantity"].as<int>();
                j["product_id"] = row["product_id"].as<int>();
                j["title"] = row["title"].as<std::string>();
                j["price"] = row["price"].as<double>();
                j["stock"] = row["stock"].as<int>();
                double line = row["price"].as<double>() * row["quantity"].as<int>();
                j["line_total"] = line;
                total += line;
                arr.append(j);
            }
            Json::Value out;
            out["ok"] = true;
            out["items"] = arr;
            out["total"] = total;
            callback(HttpResponse::newHttpJsonResponse(out));
        },
        [callback](const drogon::orm::DrogonDbException &e) {
            sendError(callback, std::string("DB error: ") + e.base().what());
        },
        buyerId);
}

// POST /api/cart {product_id, quantity}
void CartController::add(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&callback)
{
    int buyerId = getBuyerId(req);
    if (buyerId < 0)
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
    int productId = (*json).get("product_id", 0).asInt();
    int qty = (*json).get("quantity", 1).asInt();
    if (productId <= 0 || qty <= 0)
    {
        sendError(callback, "product_id and quantity (>0) required");
        return;
    }

    auto db = drogon::app().getDbClient("default");
    // Check product exists + stock.
    db->execSqlAsync(
        "SELECT stock FROM products WHERE id = ?",
        [callback, db, buyerId, productId, qty](const drogon::orm::Result &r) {
            if (r.empty())
            {
                sendError(callback, "Product not found", drogon::k404NotFound);
                return;
            }
            int stock = r[0]["stock"].as<int>();
            if (qty > stock)
            {
                sendError(callback, "Not enough stock (only " + std::to_string(stock) + ")");
                return;
            }
            // Does cart row already exist? If yes, increase quantity.
            db->execSqlAsync(
                "SELECT id, quantity FROM cart_items WHERE buyer_id = ? AND product_id = ?",
                [callback, db, buyerId, productId, qty, stock](const drogon::orm::Result &r2) {
                    if (r2.empty())
                    {
                        db->execSqlAsync(
                            "INSERT INTO cart_items (buyer_id, product_id, quantity) VALUES (?, ?, ?)",
                            [callback](const drogon::orm::Result &) {
                                Json::Value j;
                                j["ok"] = true;
                                j["message"] = "Added to cart";
                                callback(HttpResponse::newHttpJsonResponse(j));
                            },
                            [callback](const drogon::orm::DrogonDbException &e) {
                                sendError(callback, std::string("DB error: ") + e.base().what());
                            },
                            buyerId, productId, qty);
                    }
                    else
                    {
                        int rowId = r2[0]["id"].as<int>();
                        int newQty = r2[0]["quantity"].as<int>() + qty;
                        if (newQty > stock)
                        {
                            sendError(callback, "Not enough stock (only " + std::to_string(stock) + ")");
                            return;
                        }
                        db->execSqlAsync(
                            "UPDATE cart_items SET quantity = ? WHERE id = ?",
                            [callback](const drogon::orm::Result &) {
                                Json::Value j;
                                j["ok"] = true;
                                j["message"] = "Cart updated";
                                callback(HttpResponse::newHttpJsonResponse(j));
                            },
                            [callback](const drogon::orm::DrogonDbException &e) {
                                sendError(callback, std::string("DB error: ") + e.base().what());
                            },
                            newQty, rowId);
                    }
                },
                [callback](const drogon::orm::DrogonDbException &e) {
                    sendError(callback, std::string("DB error: ") + e.base().what());
                },
                buyerId, productId);
        },
        [callback](const drogon::orm::DrogonDbException &e) {
            sendError(callback, std::string("DB error: ") + e.base().what());
        },
        productId);
}

// PUT /api/cart/{id} {quantity}
void CartController::update(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&callback,
    int id)
{
    int buyerId = getBuyerId(req);
    if (buyerId < 0)
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
    int qty = (*json).get("quantity", 0).asInt();
    if (qty <= 0)
    {
        sendError(callback, "quantity must be > 0 (use Delete to remove)");
        return;
    }

    auto db = drogon::app().getDbClient("default");
    // Verify ownership + stock in one query.
    db->execSqlAsync(
        "SELECT c.id, p.stock FROM cart_items c JOIN products p ON p.id = c.product_id "
        "WHERE c.id = ? AND c.buyer_id = ?",
        [callback, db, id, qty](const drogon::orm::Result &r) {
            if (r.empty())
            {
                sendError(callback, "Cart item not found", drogon::k404NotFound);
                return;
            }
            int stock = r[0]["stock"].as<int>();
            if (qty > stock)
            {
                sendError(callback, "Not enough stock (only " + std::to_string(stock) + ")");
                return;
            }
            db->execSqlAsync(
                "UPDATE cart_items SET quantity = ? WHERE id = ?",
                [callback](const drogon::orm::Result &) {
                    Json::Value j;
                    j["ok"] = true;
                    j["message"] = "Quantity updated";
                    callback(HttpResponse::newHttpJsonResponse(j));
                },
                [callback](const drogon::orm::DrogonDbException &e) {
                    sendError(callback, std::string("DB error: ") + e.base().what());
                },
                qty, id);
        },
        [callback](const drogon::orm::DrogonDbException &e) {
            sendError(callback, std::string("DB error: ") + e.base().what());
        },
        id, buyerId);
}

// DELETE /api/cart/{id}
void CartController::remove(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&callback,
    int id)
{
    int buyerId = getBuyerId(req);
    if (buyerId < 0)
    {
        sendError(callback, "Buyer login required", drogon::k401Unauthorized);
        return;
    }
    auto db = drogon::app().getDbClient("default");
    db->execSqlAsync(
        "DELETE FROM cart_items WHERE id = ? AND buyer_id = ?",
        [callback](const drogon::orm::Result &r) {
            if (r.affectedRows() == 0)
            {
                sendError(callback, "Cart item not found", drogon::k404NotFound);
                return;
            }
            Json::Value j;
            j["ok"] = true;
            j["message"] = "Removed from cart";
            callback(HttpResponse::newHttpJsonResponse(j));
        },
        [callback](const drogon::orm::DrogonDbException &e) {
            sendError(callback, std::string("DB error: ") + e.base().what());
        },
        id, buyerId);
}
