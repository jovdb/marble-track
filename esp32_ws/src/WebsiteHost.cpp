#include "Logging.h"
#include "WebsiteHost.h"
#include <functional>

WebsiteHost::WebsiteHost(Network *networkInstance)
    : network(networkInstance), server(nullptr)
{
}

void WebsiteHost::setupRoutes()
{
    if (server == nullptr)
        return;

    // Captive Portal Detection and Windows/Proxy Probe Short-circuit
    // These routes handle the automatic connectivity checks that devices make
    server->on("/generate_204", HTTP_GET, [this](AsyncWebServerRequest *request)
               {
    MLOG_INFO("Android captive portal check - redirecting to main page");
        request->redirect("http://" + network->getIPAddress().toString() + "/"); });

    server->on("/fwlink", HTTP_GET, [this](AsyncWebServerRequest *request)
               {
    MLOG_INFO("Windows captive portal check - redirecting to main page");
        request->redirect("http://" + network->getIPAddress().toString() + "/"); });

    server->on("/hotspot-detect.html", HTTP_GET, [this](AsyncWebServerRequest *request)
               {
    MLOG_INFO("iOS captive portal check - redirecting to main page");
        request->redirect("http://" + network->getIPAddress().toString() + "/"); });

    server->on("/connectivity-check.html", HTTP_GET, [this](AsyncWebServerRequest *request)
               {
    MLOG_INFO("Generic captive portal check - redirecting to main page");
        request->redirect("http://" + network->getIPAddress().toString() + "/"); });

    // Windows/Proxy probe files: short-circuit to avoid LittleFS and watchdog resets
    server->on("/connecttest.txt", HTTP_ANY, [](AsyncWebServerRequest *request)
               {
    MLOG_INFO("Windows connecttest.txt probe - short-circuit 200 OK");
        request->send(200, "text/plain", "OK"); });
    server->on("/wpad.dat", HTTP_ANY, [](AsyncWebServerRequest *request)
               {
    MLOG_INFO("Windows wpad.dat probe - short-circuit 200 OK");
        request->send(200, "text/plain", "OK"); });

    // Web Server Root URL with debugging
    server->on("/", HTTP_GET, [this](AsyncWebServerRequest *request)
               {
    MLOG_INFO("Website accessed");
        if (LittleFS.exists("/index.html")) {
            request->send(LittleFS, "/index.html", "text/html");
        } else {
            MLOG_WARN("index.html NOT found in LittleFS. Did you upload the website?");
            
            // Create a simple fallback page with network status
            String html = "<!DOCTYPE html><html><head><title>Marble Track Control</title></head><body>";
            html += "<h1>Marble Track Control System</h1>";
            html += "<p>Web interface files not found in flash memory.</p>";
            html += "<p>Please upload the website files using PlatformIO 'Upload Filesystem Image'.</p>";
            html += "<h2>Connection Status:</h2>";
            html += "<p>" + network->getConnectionInfo() + "</p>";

            // Links
            html += "<a href='./littlefs'>LittleFS</a><br/>";
            
            html += "</body></html>";
            request->send(200, "text/html", html);
        } });

       // LittleFS file browser
    server->on("/littlefs", HTTP_GET, [this](AsyncWebServerRequest *request)
               {
        String html = "<!DOCTYPE html><html><head><title>LittleFS Files</title><style>table { border-collapse: collapse; } th, td { border: 1px solid black; padding: 5px; } .size-col { text-align: right; }</style></head><body>";
        html += "<h1>LittleFS File System</h1>";
        html += "<table><tr><th>Name</th><th class='size-col'>Size</th><th>Modified</th></tr>";
        auto getDate = [](time_t t) -> String {
            struct tm *tm = localtime(&t);
            char buf[40];
            strftime(buf, sizeof(buf), "%B %d, %Y %I:%M:%S %p", tm);
            return String(buf);
        };
        std::function<void(const String&, int)> listFiles = [&](const String& path, int indent) {
            File dir = LittleFS.open(path);
            if (!dir || !dir.isDirectory()) return;
            File file = dir.openNextFile();
            while (file) {
                String filePath = (path == "/") ? (String("/") + file.name()) : (path + "/" + file.name());
                html += "<tr><td style='padding-left: " + String(indent * 20) + "px;'>";
                if (file.isDirectory()) {
                    html += String(file.name()) + "/";
                } else {
                    html += "<a href='../" + filePath.substring(1) + "'>" + String(file.name()) + "</a>";
                }
                html += "</td><td class='size-col'>";
                if (file.isDirectory()) {
                    html += "-";
                } else {
                    html += String(file.size()) + " bytes";
                }
                html += "</td><td>";
                if (file.isDirectory()) {
                    html += "-";
                } else {
                    html += getDate(file.getLastWrite());
                }
                html += "</td></tr>";
                if (file.isDirectory()) {
                    listFiles(filePath, indent + 1);
                }
                file = dir.openNextFile();
            }
        };
        listFiles("/", 0);
        html += "</table></body></html>";
        request->send(200, "text/html", html); });

    // Test WebSocket connectivity endpoint
    server->on("/test-ws", HTTP_GET, [this](AsyncWebServerRequest *request)
               {
        String html = "<!DOCTYPE html><html><head><title>WebSocket Test</title></head><body>";
        html += "<h1>WebSocket Connection Test</h1>";
        html += "<div id='status'>Connecting...</div>";
        html += "<div id='messages'></div>";
        html += "<script>";
        html += "const ws = new WebSocket('ws://" + network->getIPAddress().toString() + "/ws');";
        html += "const status = document.getElementById('status');";
        html += "const messages = document.getElementById('messages');";
        html += "ws.onopen = () => { status.textContent = 'Connected!'; status.style.color = 'green'; };";
        html += "ws.onclose = () => { status.textContent = 'Disconnected'; status.style.color = 'red'; };";
        html += "ws.onerror = (e) => { status.textContent = 'Error: ' + e; status.style.color = 'red'; };";
        html += "ws.onmessage = (e) => { messages.innerHTML += '<div>Received: ' + e.data + '</div>'; };";
        html += "</script></body></html>";
        request->send(200, "text/html", html); });

    // Catch-all handler for any other requests (prevents LittleFS errors)
    server->onNotFound([this](AsyncWebServerRequest *request)
                       {
        String url = request->url();
    MLOG_WARN("404 - File not found: %s - redirecting to root", url.c_str());
        
        // If it's a captive portal check we missed, redirect to root
        if (url.indexOf("204") != -1 || 
            url.indexOf("generate") != -1 ||
            url.indexOf("hotspot") != -1 ||
            url.indexOf("captive") != -1 ||
            url.indexOf("connectivity") != -1) {
            request->redirect("http://" + network->getIPAddress().toString() + "/");
        } else {
            // For other missing files, return a simple redirect to root
            request->redirect("http://" + network->getIPAddress().toString() + "/");
        } });

    server->serveStatic("/", LittleFS, "/");
}

void WebsiteHost::setup(AsyncWebServer &serverRef)
{
    server = &serverRef;
    setupRoutes();
}
