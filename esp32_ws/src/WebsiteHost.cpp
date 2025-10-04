#include "Logging.h"
#include "WebsiteHost.h"
#include <Update.h>

namespace
{
    const char *otaErrorToString(uint8_t error)
    {
        switch (error)
        {
        case UPDATE_ERROR_OK:
            return "No error";
        case UPDATE_ERROR_WRITE:
            return "Flash write error";
        case UPDATE_ERROR_ERASE:
            return "Flash erase error";
        case UPDATE_ERROR_SPACE:
            return "Not enough space";
        case UPDATE_ERROR_SIZE:
            return "Image too large";
        case UPDATE_ERROR_STREAM:
            return "Stream read error";
        case UPDATE_ERROR_MD5:
            return "MD5 mismatch";
#ifdef UPDATE_ERROR_READ
        case UPDATE_ERROR_READ:
            return "Flash read error";
#endif
#ifdef UPDATE_ERROR_PARAM
        case UPDATE_ERROR_PARAM:
            return "Invalid parameter";
#endif
#ifdef UPDATE_ERROR_SIGN
        case UPDATE_ERROR_SIGN:
            return "Signature verification failed";
#endif
#ifdef UPDATE_ERROR_FLASH_CONFIG
        case UPDATE_ERROR_FLASH_CONFIG:
            return "Flash configuration error";
#endif
#ifdef UPDATE_ERROR_NEW_FLASH_CONFIG
        case UPDATE_ERROR_NEW_FLASH_CONFIG:
            return "New flash configuration required";
#endif
#ifdef UPDATE_ERROR_MAGIC_BYTE
        case UPDATE_ERROR_MAGIC_BYTE:
            return "Magic byte mismatch";
#endif
#ifdef UPDATE_ERROR_NO_PARTITION
        case UPDATE_ERROR_NO_PARTITION:
            return "Target partition not found";
#endif
#ifdef UPDATE_ERROR_BAD_ADDRESS
        case UPDATE_ERROR_BAD_ADDRESS:
            return "Invalid flash address";
#endif
#ifdef UPDATE_ERROR_PARTITION
        case UPDATE_ERROR_PARTITION:
            return "Partition handling error";
#endif
#ifdef UPDATE_ERROR_ROLLBACK
        case UPDATE_ERROR_ROLLBACK:
            return "Rollback failed";
#endif
#ifdef UPDATE_ERROR_OUT_OF_SEQUENCE
        case UPDATE_ERROR_OUT_OF_SEQUENCE:
            return "Chunks received out of sequence";
#endif
#ifdef UPDATE_ERROR_ALREADY_RUNNING
        case UPDATE_ERROR_ALREADY_RUNNING:
            return "Update already running";
#endif
        case UPDATE_ERROR_ABORT:
            return "Update aborted";
        default:
            return "Unknown error";
        }
    }

    struct OtaUploadContext
    {
        bool received = false;
        bool success = false;
        String errorMessage;
        size_t totalBytes = 0;
    } otaUploadContext;
}

WebsiteHost::WebsiteHost(Network *networkInstance)
    : network(networkInstance), server(nullptr)
{
}

void WebsiteHost::setupRoutes()
{
    if (server == nullptr)
        return;

    // Simple OTA firmware upload page and handler
    server->on("/ota", HTTP_GET, [](AsyncWebServerRequest *request)
               {
                   const char *html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
  <head>
    <meta charset="utf-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1" />
    <title>Marble Track OTA Update</title>
    <style>
      body { font-family: "Segoe UI", Arial, sans-serif; margin: 0; padding: 2rem; background: #0f172a; color: #e2e8f0; }
      h1 { margin-top: 0; font-size: 1.8rem; }
      p { max-width: 540px; line-height: 1.5; }
      form { background: #1e293b; border-radius: 12px; padding: 1.5rem; max-width: 540px; box-shadow: 0 20px 45px rgba(15, 23, 42, 0.35); }
      input[type="file"] { margin: 1rem 0; width: 100%; color: inherit; }
      button { background: #38bdf8; color: #0f172a; font-size: 1rem; border: none; border-radius: 8px; padding: 0.75rem 1.5rem; cursor: pointer; transition: background 0.2s ease, transform 0.1s ease; font-weight: 600; }
      button:hover { background: #0ea5e9; transform: translateY(-1px); }
      button:disabled { background: #334155; color: #94a3b8; cursor: not-allowed; transform: none; }
      #status { margin-top: 1rem; font-weight: 500; }
      a { color: #38bdf8; }
    </style>
  </head>
  <body>
    <h1>OTA Firmware Update</h1>
    <p>Choose a compiled <code>.bin</code> firmware image and upload it to replace the current firmware wirelessly. The controller will restart automatically after a successful upload.</p>
    <form id="ota-form" method="POST" action="/ota" enctype="multipart/form-data">
      <label for="firmware">Firmware file (.bin)</label>
      <input id="firmware" type="file" name="firmware" accept=".bin" required />
      <button id="upload-btn" type="submit">Upload Firmware</button>
      <div id="status"></div>
    </form>
    <script>
      const form = document.getElementById('ota-form');
      const statusBox = document.getElementById('status');
      const uploadBtn = document.getElementById('upload-btn');
      form.addEventListener('submit', async (event) => {
        event.preventDefault();
        if (!form.firmware.files.length) {
          statusBox.textContent = 'Please choose a firmware file (.bin).';
          return;
        }
        statusBox.textContent = 'Uploading firmware...';
        uploadBtn.disabled = true;
        try {
          const formData = new FormData(form);
          const response = await fetch('/ota', { method: 'POST', body: formData });
          const payload = await response.json();
          statusBox.textContent = payload.message || (response.ok ? 'Upload complete. Device will reboot.' : 'Upload failed.');
        } catch (error) {
          statusBox.textContent = `Upload failed: ${error}`;
        }
        uploadBtn.disabled = false;
      });
    </script>
  </body>
</html>
)rawliteral";
                   request->send(200, "text/html", html);
               });

    server->on("/ota", HTTP_POST,
               [](AsyncWebServerRequest *request)
               {
                   if (!otaUploadContext.received)
                   {
                       MLOG_WARN("OTA: POST received with no firmware uploaded");
                       request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"No firmware file uploaded.\"}");
                       return;
                   }

                   if (otaUploadContext.success)
                   {
                       MLOG_INFO("OTA: Firmware upload successful, scheduling reboot");
                       request->onDisconnect([]()
                                             {
                                                 MLOG_INFO("OTA: Restarting device to apply update");
                                                 delay(100);
                                                 ESP.restart();
                                             });
                       request->send(200, "application/json", "{\"status\":\"ok\",\"message\":\"Firmware update successful. Device will reboot.\"}");
                   }
                   else
                   {
                       String sanitized = otaUploadContext.errorMessage;
                       sanitized.replace('"', '\'');
                       String response = String("{\"status\":\"error\",\"message\":\"") + sanitized + "\"}";
                       MLOG_ERROR("OTA: Firmware upload failed - %s", otaUploadContext.errorMessage.c_str());
                       request->send(500, "application/json", response);
                   }

                   if (!otaUploadContext.success)
                   {
                       Update.abort();
                   }

                   otaUploadContext.received = false;
                   otaUploadContext.success = false;
                   otaUploadContext.errorMessage = "";
                   otaUploadContext.totalBytes = 0;
               },
               [](AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final)
               {
                   if (!index)
                   {
                       otaUploadContext.received = true;
                       otaUploadContext.success = false;
                       otaUploadContext.errorMessage = "";
                       otaUploadContext.totalBytes = 0;

                       MLOG_INFO("OTA: Upload started for %s", filename.c_str());
                       if (!Update.begin(UPDATE_SIZE_UNKNOWN))
                       {
                           otaUploadContext.errorMessage = String("Update begin failed: ") + otaErrorToString(Update.getError());
                           MLOG_ERROR("OTA: %s", otaUploadContext.errorMessage.c_str());
                       }
                       else
                       {
                           MLOG_INFO("OTA: Flash space reserved for update");
                       }
                   }

                   if (otaUploadContext.errorMessage.length() == 0 && len)
                   {
                       size_t written = Update.write(data, len);
                       if (written != len)
                       {
                           otaUploadContext.errorMessage = String("Write failed: ") + otaErrorToString(Update.getError());
                           MLOG_ERROR("OTA: %s", otaUploadContext.errorMessage.c_str());
                       }
                       else
                       {
                           otaUploadContext.totalBytes += written;
                       }
                   }

                   if (final)
                   {
                       if (otaUploadContext.errorMessage.length() == 0)
                       {
                           if (Update.end(true))
                           {
                               otaUploadContext.success = true;
                               MLOG_INFO("OTA: Upload finished (%u bytes)", static_cast<unsigned>(otaUploadContext.totalBytes));
                           }
                           else
                           {
                               otaUploadContext.errorMessage = String("Finalize failed: ") + otaErrorToString(Update.getError());
                               MLOG_ERROR("OTA: %s", otaUploadContext.errorMessage.c_str());
                           }
                       }

                       if (!otaUploadContext.success && otaUploadContext.errorMessage.length() != 0)
                       {
                           Update.abort();
                       }
                   }
               });

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
            
            html += "<h2>Available files in LittleFS:</h2><ul>";
            
            File root = LittleFS.open("/");
            File file = root.openNextFile();
            while (file) {
                html += "<li>" + String(file.name()) + " (" + String(file.size()) + " bytes)</li>";
                file = root.openNextFile();
            }
            html += "</ul>";
            html += "<p><a href='/debug'>Debug Info</a> | <a href='/network-status'>Network Status</a></p>";
            html += "</body></html>";
            request->send(200, "text/html", html);
        } });

    // Debug route to list files
    server->on("/debug", HTTP_GET, [](AsyncWebServerRequest *request)
               {
        String message = "LittleFS Debug:\n";
        File root = LittleFS.open("/");
        File file = root.openNextFile();
        while(file) {
            message += "File: " + String(file.name()) + " Size: " + String(file.size()) + "\n";
            file = root.openNextFile();
        }
        request->send(200, "text/plain", message); });

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
