#ifndef OTAUPLOAD_H
#define OTAUPLOAD_H

class AsyncWebServer;
class Network;

namespace OtaUpload {

void setup(Network &network, AsyncWebServer &server);
void loop();

}

#endif // OTAUPLOAD_H
