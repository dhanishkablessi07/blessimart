#include "controllers/ProductController.h"

using namespace drogon;

// --- small helpers (same style as AuthController) ---
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

static Json::Value rowToJson(const drogon::orm::Row &row)
{
    Json::Value j;
    j["id"] = row["id"].as<int>();
    j["seller_id"] = row["seller_id"].as<int>();
    j["title"] = row["title"].as<std::string>();
    j["description"] = row["description"].as<std::string>();
    j["price"] = row["price"].as<double>();
    j["stock"] = row["stock"].as<int>();
    j["category"] = row["category"].as<std::string>();
    j["image_url"] = row["image_url"].as<std::string>();
    // seller_name is joined in list/getOne queries (may be missing in others)
    try
    {
        j["seller_name"] = row["seller_name"].as<std::string>();
    }
    catch (...)
    {
    }
    return j;
}

// Returns user_id or -1 if not logged in. Role is output param.
static int getSessionUser(const drogon::HttpRequestPtr &req, std::string &roleOut)
{
    auto idOpt = req->session()->getOptional<int>("user_id");
    if (!idOpt)
        return -1;
    auto roleOpt = req->session()->getOptional<std::string>("user_role");
    roleOut = roleOpt ? *roleOpt : "";
    return *idOpt;
}

// GET /api/products?search=&category=
void ProductController::list(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&callback)
{
    std::string search = req->getParameter("search");
    std::string category = req->getParameter("category");

    std::string base =
        "SELECT p.*, u.name AS seller_name FROM products p "
        "JOIN users u ON u.id = p.seller_id";
    auto db = drogon::app().getDbClient("default");

    auto onSuccess = [callback](const drogon::orm::Result &r) {
        Json::Value arr(Json::arrayValue);
        for (auto &row : r)
            arr.append(rowToJson(row));
        Json::Value j;
        j["ok"] = true;
        j["products"] = arr;
        callback(HttpResponse::newHttpJsonResponse(j));
    };
    auto onError = [callback](const drogon::orm::DrogonDbException &e) {
        sendError(callback, std::string("DB error: ") + e.base().what());
    };

    // 4 simple branches so we can use bound params (no SQL injection).
    if (search.empty() && (category.empty() || category == "All"))
    {
        db->execSqlAsync(base + " ORDER BY p.id DESC", onSuccess, onError);
    }
    else if (!search.empty() && (category.empty() || category == "All"))
    {
        db->execSqlAsync(base + " WHERE p.title LIKE ? ORDER BY p.id DESC",
                         onSuccess, onError, "%" + search + "%");
    }
    else if (search.empty())
    {
        db->execSqlAsync(base + " WHERE p.category = ? ORDER BY p.id DESC",
                         onSuccess, onError, category);
    }
    else
    {
        db->execSqlAsync(
            base + " WHERE p.title LIKE ? AND p.category = ? ORDER BY p.id DESC",
            onSuccess, onError, "%" + search + "%", category);
    }
}

// GET /api/products/{id}
void ProductController::getOne(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&callback,
    int id)
{
    auto db = drogon::app().getDbClient("default");
    db->execSqlAsync(
        "SELECT p.*, u.name AS seller_name FROM products p "
        "JOIN users u ON u.id = p.seller_id WHERE p.id = ?",
        [callback](const drogon::orm::Result &r) {
            if (r.empty())
            {
                sendError(callback, "Product not found", drogon::k404NotFound);
                return;
            }
            Json::Value j = rowToJson(r[0]);
            j["ok"] = true;
            callback(HttpResponse::newHttpJsonResponse(j));
        },
        [callback](const drogon::orm::DrogonDbException &e) {
            sendError(callback, std::string("DB error: ") + e.base().what());
        },
        id);
}

// GET /api/seller/products (own products)
void ProductController::myProducts(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&callback)
{
    std::string role;
    int uid = getSessionUser(req, role);
    if (uid < 0 || role != "seller")
    {
        sendError(callback, "Seller login required", drogon::k401Unauthorized);
        return;
    }
    auto db = drogon::app().getDbClient("default");
    db->execSqlAsync(
        "SELECT p.*, u.name AS seller_name FROM products p "
        "JOIN users u ON u.id = p.seller_id WHERE p.seller_id = ? ORDER BY p.id DESC",
        [callback](const drogon::orm::Result &r) {
            Json::Value arr(Json::arrayValue);
            for (auto &row : r)
                arr.append(rowToJson(row));
            Json::Value j;
            j["ok"] = true;
            j["products"] = arr;
            callback(HttpResponse::newHttpJsonResponse(j));
        },
        [callback](const drogon::orm::DrogonDbException &e) {
            sendError(callback, std::string("DB error: ") + e.base().what());
        },
        uid);
}

// POST /api/seller/products
void ProductController::create(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&callback)
{
    std::string role;
    int uid = getSessionUser(req, role);
    if (uid < 0 || role != "seller")
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
    std::string title = (*json).get("title", "").asString();
    std::string description = (*json).get("description", "").asString();
    double price = (*json).get("price", 0).asDouble();
    int stock = (*json).get("stock", 0).asInt();
    std::string category = (*json).get("category", "General").asString();
    std::string image_url = (*json).get("image_url", "").asString();

    if (title.empty() || price < 0 || stock < 0)
    {
        sendError(callback, "Title, price >= 0 and stock >= 0 are required");
        return;
    }
    if (category.empty())
        category = "General";

    auto db = drogon::app().getDbClient("default");
    db->execSqlAsync(
        "INSERT INTO products (seller_id, title, description, price, stock, category, image_url) "
        "VALUES (?, ?, ?, ?, ?, ?, ?)",
        [callback](const drogon::orm::Result &) {
            Json::Value j;
            j["ok"] = true;
            j["message"] = "Product added";
            callback(HttpResponse::newHttpJsonResponse(j));
        },
        [callback](const drogon::orm::DrogonDbException &e) {
            sendError(callback, std::string("DB error: ") + e.base().what());
        },
        uid, title, description, price, stock, category, image_url);
}

// PUT /api/seller/products/{id} (only owner)
void ProductController::update(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&callback,
    int id)
{
    std::string role;
    int uid = getSessionUser(req, role);
    if (uid < 0 || role != "seller")
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

    auto db = drogon::app().getDbClient("default");
    // First check ownership.
    db->execSqlAsync(
        "SELECT seller_id FROM products WHERE id = ?",
        [req, callback, db, uid, id, json](const drogon::orm::Result &r) {
            if (r.empty())
            {
                sendError(callback, "Product not found", drogon::k404NotFound);
                return;
            }
            if (r[0]["seller_id"].as<int>() != uid)
            {
                sendError(callback, "Not your product", drogon::k403Forbidden);
                return;
            }
            std::string title = (*json).get("title", "").asString();
            std::string description = (*json).get("description", "").asString();
            double price = (*json).get("price", 0).asDouble();
            int stock = (*json).get("stock", 0).asInt();
            std::string category = (*json).get("category", "General").asString();
            std::string image_url = (*json).get("image_url", "").asString();
            if (title.empty() || price < 0 || stock < 0)
            {
                sendError(callback, "Title, price >= 0 and stock >= 0 are required");
                return;
            }
            db->execSqlAsync(
                "UPDATE products SET title=?, description=?, price=?, stock=?, category=?, image_url=? WHERE id=?",
                [callback](const drogon::orm::Result &) {
                    Json::Value j;
                    j["ok"] = true;
                    j["message"] = "Product updated";
                    callback(HttpResponse::newHttpJsonResponse(j));
                },
                [callback](const drogon::orm::DrogonDbException &e) {
                    sendError(callback, std::string("DB error: ") + e.base().what());
                },
                title, description, price, stock, category, image_url, id);
        },
        [callback](const drogon::orm::DrogonDbException &e) {
            sendError(callback, std::string("DB error: ") + e.base().what());
        },
        id);
}

// DELETE /api/seller/products/{id} (only owner)
void ProductController::remove(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&callback,
    int id)
{
    std::string role;
    int uid = getSessionUser(req, role);
    if (uid < 0 || role != "seller")
    {
        sendError(callback, "Seller login required", drogon::k401Unauthorized);
        return;
    }
    auto db = drogon::app().getDbClient("default");
    db->execSqlAsync(
        "SELECT seller_id FROM products WHERE id = ?",
        [callback, db, uid, id](const drogon::orm::Result &r) {
            if (r.empty())
            {
                sendError(callback, "Product not found", drogon::k404NotFound);
                return;
            }
            if (r[0]["seller_id"].as<int>() != uid)
            {
                sendError(callback, "Not your product", drogon::k403Forbidden);
                return;
            }
            db->execSqlAsync(
                "DELETE FROM products WHERE id = ?",
                [callback](const drogon::orm::Result &) {
                    Json::Value j;
                    j["ok"] = true;
                    j["message"] = "Product deleted";
                    callback(HttpResponse::newHttpJsonResponse(j));
                },
                [callback](const drogon::orm::DrogonDbException &e) {
                    sendError(callback, std::string("DB error: ") + e.base().what());
                },
                id);
        },
        [callback](const drogon::orm::DrogonDbException &e) {
            sendError(callback, std::string("DB error: ") + e.base().what());
        },
        id);
}
