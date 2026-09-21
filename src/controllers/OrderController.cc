#include "controllers/OrderController.h"

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

static int getBuyerId(const drogon::HttpRequestPtr &req)
{
    auto idOpt = req->session()->getOptional<int>("user_id");
    auto roleOpt = req->session()->getOptional<std::string>("user_role");
    if (!idOpt || !roleOpt || *roleOpt != "buyer")
        return -1;
    return *idOpt;
}

// POST /api/checkout {address} -- no real payment, just Cash on Delivery.
void OrderController::checkout(
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
    std::string address = json ? (*json).get("address", "").asString() : "";
    if (address.empty())
    {
        sendError(callback, "Delivery address is required");
        return;
    }

    auto db = drogon::app().getDbClient("default");

    // 1. Load cart with current prices + stock.
    db->execSqlAsync(
        "SELECT c.product_id, c.quantity, p.price, p.stock, p.seller_id "
        "FROM cart_items c JOIN products p ON p.id = c.product_id "
        "WHERE c.buyer_id = ?",
        [callback, db, buyerId, address](const drogon::orm::Result &cart) {
            if (cart.empty())
            {
                sendError(callback, "Cart is empty");
                return;
            }
            // 2. Validate stock + compute total.
            double total = 0.0;
            for (auto &row : cart)
            {
                int qty = row["quantity"].as<int>();
                int stock = row["stock"].as<int>();
                if (qty > stock)
                {
                    sendError(callback, "Not enough stock for product " +
                                            std::to_string(row["product_id"].as<int>()));
                    return;
                }
                total += row["price"].as<double>() * qty;
            }

            // 3. Create the order row.
            db->execSqlAsync(
                "INSERT INTO orders (buyer_id, total_price, status, address) "
                "VALUES (?, ?, 'placed', ?)",
                [callback, db, buyerId, cart](const drogon::orm::Result &) {
                    // 4. Get new order id (SQLite specific).
                    db->execSqlAsync(
                        "SELECT last_insert_rowid() AS id",
                        [callback, db, buyerId, cart](const drogon::orm::Result &r) {
                            int orderId = r[0]["id"].as<int>();

                            // 5. Insert each item + reduce stock, one by one.
                            // Beginner style: shared counter, finish when all done.
                            auto remaining =
                                std::make_shared<size_t>(cart.size());
                            auto failed = std::make_shared<bool>(false);

                            for (auto &row : cart)
                            {
                                int pid = row["product_id"].as<int>();
                                int qty = row["quantity"].as<int>();
                                double price = row["price"].as<double>();
                                int seller = row["seller_id"].as<int>();

                                db->execSqlAsync(
                                    "INSERT INTO order_items "
                                    "(order_id, product_id, seller_id, quantity, price_at_buy) "
                                    "VALUES (?, ?, ?, ?, ?)",
                                    [callback, db, buyerId, orderId, pid, qty,
                                     remaining, failed](const drogon::orm::Result &) {
                                        db->execSqlAsync(
                                            "UPDATE products SET stock = stock - ? WHERE id = ?",
                                            [callback, db, buyerId, orderId,
                                             remaining, failed](const drogon::orm::Result &) {
                                                if (*failed)
                                                    return;
                                                (*remaining)--;
                                                if (*remaining == 0)
                                                {
                                                    // 6. Clear cart + reply.
                                                    db->execSqlAsync(
                                                        "DELETE FROM cart_items WHERE buyer_id = ?",
                                                        [callback, orderId](const drogon::orm::Result &) {
                                                            Json::Value j;
                                                            j["ok"] = true;
                                                            j["order_id"] = orderId;
                                                            j["message"] = "Order placed (Cash on Delivery)";
                                                            callback(HttpResponse::newHttpJsonResponse(j));
                                                        },
                                                        [callback, failed](const drogon::orm::DrogonDbException &e) {
                                                            *failed = true;
                                                            sendError(callback, std::string("DB error: ") + e.base().what());
                                                        },
                                                        buyerId);
                                                }
                                            },
                                            [callback, failed](const drogon::orm::DrogonDbException &e) {
                                                if (!*failed)
                                                {
                                                    *failed = true;
                                                    sendError(callback, std::string("DB error: ") + e.base().what());
                                                }
                                            },
                                            qty, pid);
                                    },
                                    [callback, failed](const drogon::orm::DrogonDbException &e) {
                                        if (!*failed)
                                        {
                                            *failed = true;
                                            sendError(callback, std::string("DB error: ") + e.base().what());
                                        }
                                    },
                                    orderId, pid, seller, qty, price);
                            }
                        },
                        [callback](const drogon::orm::DrogonDbException &e) {
                            sendError(callback, std::string("DB error: ") + e.base().what());
                        });
                },
                [callback](const drogon::orm::DrogonDbException &e) {
                    sendError(callback, std::string("DB error: ") + e.base().what());
                },
                buyerId, total, address);
        },
        [callback](const drogon::orm::DrogonDbException &e) {
            sendError(callback, std::string("DB error: ") + e.base().what());
        },
        buyerId);
}

// GET /api/orders (buyer history)
void OrderController::myOrders(
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
        "SELECT id, total_price, status, address, created_at FROM orders "
        "WHERE buyer_id = ? ORDER BY id DESC",
        [callback](const drogon::orm::Result &r) {
            Json::Value arr(Json::arrayValue);
            for (auto &row : r)
            {
                Json::Value j;
                j["id"] = row["id"].as<int>();
                j["total_price"] = row["total_price"].as<double>();
                j["status"] = row["status"].as<std::string>();
                j["address"] = row["address"].as<std::string>();
                j["created_at"] = row["created_at"].as<std::string>();
                arr.append(j);
            }
            Json::Value out;
            out["ok"] = true;
            out["orders"] = arr;
            callback(HttpResponse::newHttpJsonResponse(out));
        },
        [callback](const drogon::orm::DrogonDbException &e) {
            sendError(callback, std::string("DB error: ") + e.base().what());
        },
        buyerId);
}

// GET /api/orders/{id} (one order + its items, owner only)
void OrderController::orderDetail(
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
        "SELECT id, total_price, status, address, created_at FROM orders "
        "WHERE id = ? AND buyer_id = ?",
        [callback, db, id](const drogon::orm::Result &r) {
            if (r.empty())
            {
                sendError(callback, "Order not found", drogon::k404NotFound);
                return;
            }
            Json::Value order;
            order["id"] = r[0]["id"].as<int>();
            order["total_price"] = r[0]["total_price"].as<double>();
            order["status"] = r[0]["status"].as<std::string>();
            order["address"] = r[0]["address"].as<std::string>();
            order["created_at"] = r[0]["created_at"].as<std::string>();

            db->execSqlAsync(
                "SELECT oi.quantity, oi.price_at_buy, p.title "
                "FROM order_items oi JOIN products p ON p.id = oi.product_id "
                "WHERE oi.order_id = ?",
                [callback, order](const drogon::orm::Result &items) {
                    Json::Value arr(Json::arrayValue);
                    for (auto &row : items)
                    {
                        Json::Value j;
                        j["title"] = row["title"].as<std::string>();
                        j["quantity"] = row["quantity"].as<int>();
                        j["price_at_buy"] = row["price_at_buy"].as<double>();
                        arr.append(j);
                    }
                    Json::Value out;
                    out["ok"] = true;
                    out["order"] = order;
                    out["items"] = arr;
                    callback(HttpResponse::newHttpJsonResponse(out));
                },
                [callback](const drogon::orm::DrogonDbException &e) {
                    sendError(callback, std::string("DB error: ") + e.base().what());
                },
                id);
        },
        [callback](const drogon::orm::DrogonDbException &e) {
            sendError(callback, std::string("DB error: ") + e.base().what());
        },
        id, buyerId);
}
