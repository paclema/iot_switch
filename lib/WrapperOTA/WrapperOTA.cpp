#include "WrapperOTA.h"


void WrapperOTA::init(SSD1306Wire *display) {
  ArduinoOTA.begin();
  ArduinoOTA.onStart([display]() {
    display->clear();
    display->setFont(ArialMT_Plain_10);
    display->setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
    display->drawString(display->getWidth()/2, display->getHeight()/2 - 10, "OTA Update");
    display->display();
  });

  ArduinoOTA.onProgress([display](unsigned int progress, unsigned int total) {
    display->drawProgressBar(4, 32, 120, 8, progress / (total / 100) );
    display->display();
  });

  ArduinoOTA.onEnd([display]() {
    display->clear();
    display->setFont(ArialMT_Plain_10);
    display->setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
    display->drawString(display->getWidth()/2, display->getHeight()/2, "Restarting...");
    display->display();
  });

};


  void WrapperOTA::init(WebConfigServer *config) {
    // Port defaults to 8266
    ArduinoOTA.setPort(8266);

    // Hostname defaults to esp8266-[ChipID]
    char hostname[sizeof(config->network.hostname)];
    config->network.hostname.toCharArray(hostname, sizeof(hostname));
    ArduinoOTA.setHostname(hostname);

    ArduinoOTA.begin();
    ArduinoOTA.onStart([config]() {
    });

    ArduinoOTA.onProgress([config](unsigned int progress, unsigned int total) {
      // Log.info("OTA Progress: %i%%", (progress / (total / 100)));
    });

    ArduinoOTA.onEnd([config]() {
    });

};
