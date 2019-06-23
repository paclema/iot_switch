#ifndef WebConfigServer_H
#define WebConfigServer_H

#include <Arduino.h>

#include <ESP8266WebServer.h>
#include <FS.h>

#define ARDUINOJSON_ENABLE_ALIGNMENT 1
#include <ArduinoJson.h>


#define CONFIG_FILE "/config.json"
#define CONFIG_JSON_SIZE 1648
#define MQTT_TOPIC_MAX_SIZE_LIST 10
#define JSON_MAX_SIZE_LIST 6

#define CONFIG_LOADED "loaded"
#define CONFIG_NOT_LOADED "not_loaded"


class WebConfigServer {

public:

  // const char* status = new char(32);
  String config_status;

  struct Network {
    String ssid_name;
    String ssid_password;
    String ip_address;
    String subnet;
    String dns_server;
    String hostname;


  } network;

  struct Mqtt {
    String server;
    int port;
    String id_name;
    bool enable_user_and_pass;
    String user_name;
    String user_password;
    bool enable_certificates;
    String ca_file;
    String cert_file;
    String key_file;
    String pub_topic[MQTT_TOPIC_MAX_SIZE_LIST];
    String sub_topic[MQTT_TOPIC_MAX_SIZE_LIST];
  } mqtt;

  struct FTP {
    bool enabled;
    String user;
    String password;
  };

  struct DeepSleep {
    bool enabled;
    String mode;
    String mode_options[JSON_MAX_SIZE_LIST];
    int sleep_time;
    int sleep_delay;
  };

  struct LightSleep {
    bool enabled;
    String mode;
    String mode_options[JSON_MAX_SIZE_LIST];
  };

  struct Services {
    FTP ftp;
    bool ota;
    DeepSleep deep_sleep;
    LightSleep light_sleep;
  } services;

  struct Device {
    bool track_restart_counter;
  } device;

  struct Info {
    int restart_counter;
    String fw_version;
    String repo;

  } info;



  WebConfigServer(void);

  void configureServer(ESP8266WebServer *server);
  void handle(void);
  bool begin(void);

  String status(void) { return config_status;};

private:

  String formatBytes(size_t bytes);

  void saveWebConfigurationFile(const char *filename, const JsonDocument& doc);
  void parseConfig(const JsonDocument& doc);

  void loadConfigurationFile(const char *filename);
  void saveConfigurationFile(const char *filename);
  void printFile(String filename);
  void restoreBackupFile(String filenamechar);
  // void updateGpio(void);
  void updateGpio(ESP8266WebServer *server);


};
#endif
