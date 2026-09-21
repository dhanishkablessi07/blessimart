#pragma once
#include <drogon/HttpController.h>

// Handles: register, login, logout, me.
// Routes are registered with ADD_METHOD_TO below.
class AuthController : public drogon::HttpController<AuthController>
{
  public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(AuthController::registerUser, "/api/register", drogon::Post);
    ADD_METHOD_TO(AuthController::login, "/api/login", drogon::Post);
    ADD_METHOD_TO(AuthController::logout, "/api/logout", drogon::Post);
    ADD_METHOD_TO(AuthController::me, "/api/me", drogon::Get);
    METHOD_LIST_END

    void registerUser(const drogon::HttpRequestPtr &req,
                      std::function<void(const drogon::HttpResponsePtr &)> &&callback);
    void login(const drogon::HttpRequestPtr &req,
               std::function<void(const drogon::HttpResponsePtr &)> &&callback);
    void logout(const drogon::HttpRequestPtr &req,
                std::function<void(const drogon::HttpResponsePtr &)> &&callback);
    void me(const drogon::HttpRequestPtr &req,
            std::function<void(const drogon::HttpResponsePtr &)> &&callback);
};
