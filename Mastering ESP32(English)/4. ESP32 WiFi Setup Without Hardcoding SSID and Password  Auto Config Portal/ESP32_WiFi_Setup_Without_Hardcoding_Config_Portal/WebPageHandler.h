#ifndef WEBPAGE_HANDLER_H
#define WEBPAGE_HANDLER_H

#include <WebServer.h>
#include <Preferences.h>

class WebPageHandler {
public:
  WebPageHandler(WebServer& server);
  void begin();

private:
  WebServer& _server;
  Preferences _prefs;
  
  // Variables for connection testing
  bool isTestingConnection = false;
  unsigned long testStartTime = 0;

  // HTML Content
  static const char* indexHtml;

  // Handlers
  void handleRoot();      
  void handleScan();      
  void handleSave();      
  void handleGetStatus(); 
  void handleCheckConnect(); // New: Checks if the new WiFi is working
};

#endif