#include "controllers/AdminController.h"

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

static bool isAdmin(const drogon::HttpRequestPtr &req)
{
    auto roleOpt = req->session()->getOptional<std::string>("user_role");
    return roleOpt && *roleOpt == "admin";
}

// GET /api/admin/users
void AdminController::users(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&callback)
{
    if (!isAdmin(req))
    {
        sendError(callback, "Admin login required", drogon::k401Unauthorized);
        return;
    }
    auto db = drogon::app().getDbClient("default");
    db->execSqlAsync(
        "SELECT id, name, email, role, created_at FROM users ORDER BY id",
        [callback](const drogon::orm::Result &r) {
            Json::Value arr(Json::arrayValue);
            for (auto &row : r)
            {
                Json::Value j;
                j["id"] = row["id"].as<int>();
                j["name"] = row["name"].as<std::string>();
                j["email"] = row["email"].as<std::string>();
                j["role"] = row["role"].as<std::string>();
                j["created_at"] = row["created_at"].as<std::string>();
                arr.append(j);
            }
            Json::Value out;
            out["ok"] = true;
            out["users"] = arr;
            callback(HttpResponse::newHttpJsonResponse(out));
        },
        [callback](const drogon::orm::DrogonDbException &e) {
            sendError(callback, std::string("DB error: ") + e.base().what());
        });
}

// GET /api/admin/orders
void AdminController::orders(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&callback)
{
    if (!isAdmin(req))
    {
        sendError(callback, "Admin login required", drogon::k401Unauthorized);
        return;
    }
    auto db = drogon::app().getDbClient("default");
    db->execSqlAsync(
        "SELECT o.id, o.total_price, o.status, o.address, o.created_at, u.name AS buyer_name "
        "FROM orders o JOIN users u ON u.id = o.buyer_id ORDER BY o.id DESC",
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
                j["buyer_name"] = row["buyer_name"].as<std::string>();
                arr.append(j);
            }
            Json::Value out;
            out["ok"] = true;
            out["orders"] = arr;
            callback(HttpResponse::newHttpJsonResponse(out));
        },
        [callback](const drogon::orm::DrogonDbException &e) {
            sendError(callback, std::string("DB error: ") + e.base().what());
        });
}

// DELETE /api/admin/products/{id}
void AdminController::deleteProduct(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&callback,
    int productId)
{
    if (!isAdmin(req))
    {
        sendError(callback, "Admin login required", drogon::k401Unauthorized);
        return;
    }
    auto db = drogon::app().getDbClient("default");
    db->execSqlAsync(
        "DELETE FROM products WHERE id = ?",
        [callback](const drogon::orm::Result &r) {
            if (r.affectedRows() == 0)
            {
                sendError(callback, "Product not found", drogon::k404NotFound);
                return;
            }
            Json::Value j;
            j["ok"] = true;
            j["message"] = "Product removed by admin";
            callback(HttpResponse::newHttpJsonResponse(j));
        },
        [callback](const drogon::orm::DrogonDbException &e) {
            sendError(callback, std::string("DB error: ") + e.base().what());
        },
        productId);
}
