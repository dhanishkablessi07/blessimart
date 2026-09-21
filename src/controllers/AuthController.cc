#include "controllers/AuthController.h"
#include <drogon/utils/Utilities.h>

using namespace drogon;

// Small helper: send {"ok":false,"message":"..."} with status code.
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

static void sendOk(auto &&callback, const Json::Value &data)
{
    auto resp = HttpResponse::newHttpJsonResponse(data);
    callback(resp);
}

void AuthController::registerUser(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&callback)
{
    auto json = req->getJsonObject();
    if (!json)
    {
        sendError(callback, "Invalid JSON body");
        return;
    }

    std::string name = (*json).get("name", "").asString();
    std::string email = (*json).get("email", "").asString();
    std::string password = (*json).get("password", "").asString();
    std::string role = (*json).get("role", "buyer").asString();

    // Basic validation (beginner-friendly, no regex).
    if (name.empty() || email.empty() || password.empty())
    {
        sendError(callback, "Name, email and password are required");
        return;
    }
    if (password.size() < 4)
    {
        sendError(callback, "Password must be at least 4 characters");
        return;
    }
    if (role != "buyer" && role != "seller")
    {
        sendError(callback, "Role must be buyer or seller");
        return;
    }

    std::string hash = drogon::utils::getSha256(password);
    auto db = drogon::app().getDbClient("default");

    // Step 1: check email is not already used.
    db->execSqlAsync(
        "SELECT id FROM users WHERE email=?",
        [callback, db, name, email, hash, role](const drogon::orm::Result &r) {
            if (!r.empty())
            {
                sendError(callback, "Email already registered", drogon::k400BadRequest);
                return;
            }
            // Step 2: insert new user.
            db->execSqlAsync(
                "INSERT INTO users (name, email, password_hash, role) VALUES (?, ?, ?, ?)",
                [callback](const drogon::orm::Result &) {
                    Json::Value j;
                    j["ok"] = true;
                    j["message"] = "Registered! Please login.";
                    sendOk(callback, j);
                },
                [callback](const drogon::orm::DrogonDbException &e) {
                    sendError(callback, std::string("DB error: ") + e.base().what());
                },
                name, email, hash, role);
        },
        [callback](const drogon::orm::DrogonDbException &e) {
            sendError(callback, std::string("DB error: ") + e.base().what());
        },
        email);
}

void AuthController::login(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&callback)
{
    auto json = req->getJsonObject();
    if (!json)
    {
        sendError(callback, "Invalid JSON body");
        return;
    }

    std::string email = (*json).get("email", "").asString();
    std::string password = (*json).get("password", "").asString();
    if (email.empty() || password.empty())
    {
        sendError(callback, "Email and password are required");
        return;
    }

    std::string hash = drogon::utils::getSha256(password);
    auto db = drogon::app().getDbClient("default");

    db->execSqlAsync(
        "SELECT id, name, email, role, password_hash FROM users WHERE email=?",
        [req, callback, hash](const drogon::orm::Result &r) {
            if (r.empty())
            {
                sendError(callback, "Invalid email or password", drogon::k401Unauthorized);
                return;
            }
            auto row = r[0];
            std::string dbHash = row["password_hash"].as<std::string>();
            if (dbHash != hash)
            {
                sendError(callback, "Invalid email or password", drogon::k401Unauthorized);
                return;
            }
            int id = row["id"].as<int>();
            std::string name = row["name"].as<std::string>();
            std::string email = row["email"].as<std::string>();
            std::string role = row["role"].as<std::string>();

            // Save login in Drogon session (cookie-based).
            req->session()->insert("user_id", id);
            req->session()->insert("user_role", role);
            req->session()->insert("user_name", name);

            Json::Value j;
            j["ok"] = true;
            j["id"] = id;
            j["name"] = name;
            j["email"] = email;
            j["role"] = role;
            sendOk(callback, j);
        },
        [callback](const drogon::orm::DrogonDbException &e) {
            sendError(callback, std::string("DB error: ") + e.base().what());
        },
        email);
}

void AuthController::logout(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&callback)
{
    req->session()->clear();
    Json::Value j;
    j["ok"] = true;
    j["message"] = "Logged out";
    sendOk(callback, j);
}

void AuthController::me(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&callback)
{
    auto idOpt = req->session()->getOptional<int>("user_id");
    if (!idOpt)
    {
        sendError(callback, "Not logged in", drogon::k401Unauthorized);
        return;
    }

    auto db = drogon::app().getDbClient("default");
    db->execSqlAsync(
        "SELECT id, name, email, role FROM users WHERE id=?",
        [callback](const drogon::orm::Result &r) {
            if (r.empty())
            {
                sendError(callback, "User not found", drogon::k401Unauthorized);
                return;
            }
            auto row = r[0];
            Json::Value j;
            j["ok"] = true;
            j["id"] = row["id"].as<int>();
            j["name"] = row["name"].as<std::string>();
            j["email"] = row["email"].as<std::string>();
            j["role"] = row["role"].as<std::string>();
            sendOk(callback, j);
        },
        [callback](const drogon::orm::DrogonDbException &e) {
            sendError(callback, std::string("DB error: ") + e.base().what());
        },
        *idOpt);
}
