#ifndef WrapperOTA_H
#define WrapperOTA_H

#include <Arduino.h>

#include "SSD1306Wire.h" // legacy include: `#include "SSD1306.h"`

// OTA Includes
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>

#include <WebConfigServer.h>

class WrapperOTA{

public:

  void init(SSD1306Wire *display);
  void init(WebConfigServer *config);
  void handle(void) {
    ArduinoOTA.handle();
  }

};

#endif
