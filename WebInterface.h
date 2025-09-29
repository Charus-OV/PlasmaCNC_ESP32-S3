#ifndef WEBINTERFACE_H
#define WEBINTERFACE_H

#include <WebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include <WiFi.h>

class WebInterface {
public:
    static void init();
    static void handleClient();
    static void sendSystemState();
    
private:
    static WebServer server;
    static WebSocketsServer webSocket;
    
    static void handleRoot();
    static void handleAPIStatus();
    static void handleAPIFiles();
    static void handleFileUpload();
    static void handleWebSocket(uint8_t num, WStype_t type, uint8_t* payload, size_t length);
    static void processWebSocketCommand(String message);
    static String getMainPage();
};

#endif