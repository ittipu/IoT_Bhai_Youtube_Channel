#ifndef WEBPAGE_HANDLER_H
#define WEBPAGE_HANDLER_H

#include <WebServer.h>
#include <Preferences.h>

class WebPageHandler {
public:
  WebPageHandler(WebServer& server);
  void begin();
  bool isConfigDone();

private:
  WebServer& _server;
  Preferences _prefs;
  bool configDone = false;

  static const char* htmlForm;
  void handleRoot();
  void handleSubmit();
  String buildSuccessPage();
};


#endif
