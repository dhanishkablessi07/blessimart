#include <drogon/drogon.h>

int main()
{
    // Load settings from config.json (port, frontend folder, sqlite db).
    // IMPORTANT: Run the program from the BlessiMart/ folder so it can
    // find config.json and frontend/.
    drogon::app().loadConfigFile("config.json");

    // Simple test route: open http://localhost:8080/ping
    drogon::app().registerHandler(
        "/ping",
        [](const drogon::HttpRequestPtr &,
           std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setBody("BlessiMart server is running!");
            callback(resp);
        });

    // Serve frontend/ files automatically (index.html, css, js).
    drogon::app().run();
    return 0;
}
