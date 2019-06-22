# iot_switch

This repo is comming from https://github.com/paclema/iot_button
Probably in the future they will be merged.

### Features:

* Store configs as json and certs on SPI Flash File System (SPIFFS).
* ftp server to easily modify files stored on SPIFFS.
* Boootstrap Web server.
* MQTT connection.
* OTA updates.

### Requirements:

* wemos d1 mini, nodemcuv2 or esp12
* Connect Reed Switch between 3v3 and D1
* Conect 10k R between GND and D1
