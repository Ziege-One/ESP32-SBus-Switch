#pragma once
#include <WiFi.h>

class WebServerManager {
public:
  // begin startet AP (oder verbindet, je nach Implementierung) und startet den Server
  static void begin(const char* apSsid, const char* apPassword);

  // in loop() aufrufen, bearbeitet anstehende Clients
  static void Webpage();

private:
  static WiFiServer server;
  static String header;
  static String valueString;
  static int pos1;
  static int pos2;
  static int Menu;
  static unsigned long previousTime;
  static unsigned long currentTime;
  static const long timeoutTime;
};
