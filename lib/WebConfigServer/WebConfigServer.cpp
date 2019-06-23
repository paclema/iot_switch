#include "WebConfigServer.h"


WebConfigServer::WebConfigServer(void){
  // Serial.println("WebConfigServer loaded");
  config_status = CONFIG_NOT_LOADED;
}

bool WebConfigServer::begin(void){

  if (!SPIFFS.begin()) {
    Serial.println("SPIFFS Mount failed");
    config_status = CONFIG_NOT_LOADED;
    return false;
  } else {
    Serial.println("SPIFFS Mount succesfull");
    Dir dir = SPIFFS.openDir("/");
    while (dir.next()) {
      String fileName = dir.fileName();
      size_t fileSize = dir.fileSize();
      Serial.printf("FS File: %s, size: %s\n", fileName.c_str(), formatBytes(fileSize).c_str());
    }
    Serial.printf("\n");

    if (SPIFFS.exists(CONFIG_FILE)) {
      Serial.print(CONFIG_FILE); Serial.println(" exists!");
      loadConfigurationFile(CONFIG_FILE);
    // printFile(CONFIG_FILE);
  } else {
    config_status = CONFIG_NOT_LOADED;
    return false;
    }
  }
  config_status = CONFIG_LOADED;
  return true;

}


// Format bytes:
String WebConfigServer::formatBytes(size_t bytes){
  if (bytes < 1024) {
    return String(bytes) + " B";
  } else if (bytes < (1024 * 1024)) {
    return String(bytes / 1024.0) + " KB";
  } else if (bytes < (1024 * 1024 * 1024)) {
    return String(bytes / 1024.0 / 1024.0) + " MB";
  } else {
    return String(bytes / 1024.0 / 1024.0 / 1024.0) + " GB";
  }
}


// Saves the web configuration from a POST req to a file
void WebConfigServer::saveWebConfigurationFile(const char *filename, const JsonDocument& doc){
  // Delete existing file, otherwise the configuration is appended to the file
  SPIFFS.remove(filename);

  // Open file for writing
  // File file = SD.open(filename, FILE_WRITE);
  File file = SPIFFS.open(filename, "w");
  if (!file) {
    Serial.println(F("Failed to create file"));
    return;
  }

  // Serialize JSON to file
  if (serializeJson(doc, file) == 0) {
    Serial.println(F("Failed to write to file"));
  }

  // Close the file
  file.close();
}


void WebConfigServer::parseConfig(const JsonDocument& doc){

    // serializeJsonPretty(doc, Serial);

    // Parse char network[64]:
    // strlcpy(network.ssid_name, doc["network"]["ssid_name"] | "SSID_name", sizeof(network.ssid_name));

    // Network object:
    network.ssid_name = doc["network"]["ssid_name"] | "SSID_name";
    network.ssid_password = doc["network"]["ssid_password"] | "SSID_password";
    network.ip_address = doc["network"]["ip_address"] | "192.168.1.2";
    network.subnet = doc["network"]["subnet"] | "255.255.255.0";
    network.dns_server = doc["network"]["dns_server"] | "192.168.1.1";
    network.hostname = doc["network"]["hostname"] | "iotdevice.local";


    // MQTT object:
    mqtt.server= doc["mqtt"]["server"] | "server_address";
    mqtt.port = doc["mqtt"]["port"] | 8888;
    mqtt.id_name= doc["mqtt"]["id_name"] | "iotdevice";
    mqtt.enable_user_and_pass = doc["mqtt"]["enable_user_and_pass"] | false;
    mqtt.user_name= doc["mqtt"]["user_name"] | "user_name";
    mqtt.user_password= doc["mqtt"]["user_password"] | "user_password";
    mqtt.enable_certificates = doc["mqtt"]["enable_certificates"] | false;
    mqtt.ca_file= doc["mqtt"]["ca_file"] | "certs/ca.crt";
    mqtt.cert_file= doc["mqtt"]["cert_file"] | "certs/client.crt";
    mqtt.key_file= doc["mqtt"]["key_file"] | "certs/client.key";
    mqtt.ca_file= doc["mqtt"]["ca_file"] | "server_address";
    for (int i = 0; i < doc["mqtt"]["pub_topic"].size(); i++) { //Iterate through results
      // mqtt.pub_topic[i] = doc["mqtt"]["pub_topic"][i];  //Implicit cast
      mqtt.pub_topic[i] = doc["mqtt"]["pub_topic"][i].as<String>(); //Explicit cast
    }
    for (int i = 0; i < doc["mqtt"]["sub_topic"].size(); i++)
      mqtt.sub_topic[i] = doc["mqtt"]["sub_topic"][i].as<String>();


    // Services object:
    // FTP
    services.ftp.enabled = doc["services"]["FTP"]["enabled"] | false;
    services.ftp.user = doc["services"]["FTP"]["user"] | "admin";
    services.ftp.password = doc["services"]["FTP"]["password"] | "admin";
    // OTA
    services.ota = doc["services"]["OTA"] | false;
    // DeepSleep
    services.deep_sleep.enabled = doc["services"]["deep_sleep"]["enabled"] | false;
    services.deep_sleep.mode = doc["services"]["deep_sleep"]["mode"] | "WAKE_RF_DEFAULT";
    for (int i = 0; i < doc["services"]["deep_sleep"]["mode_options"].size(); i++) { //Iterate through results
      services.deep_sleep.mode_options[i] = doc["services"]["deep_sleep"]["mode_options"][i].as<String>(); //Explicit cast
    }
    services.deep_sleep.sleep_time = doc["services"]["deep_sleep"]["sleep_time"] | 10;
    services.deep_sleep.sleep_delay = doc["services"]["deep_sleep"]["sleep_delay"] | 5;
    // LightSleep
    services.light_sleep.enabled = doc["services"]["light_sleep"]["enabled"] | false;
    services.light_sleep.mode = doc["services"]["light_sleep"]["mode"] | "LIGHT_SLEEP_T";
    for (int i = 0; i < doc["services"]["light_sleep"]["mode_options"].size(); i++) { //Iterate through results
      services.light_sleep.mode_options[i] = doc["services"]["light_sleep"]["mode_options"][i].as<String>(); //Explicit cast
    }

    // Device object:
    device.track_restart_counter = doc["device"]["track_restart_counter"] | true;


    // Info object:
    info.restart_counter = doc["info"]["ftp"]["enabled"] | false;
    info.fw_version = doc["info"]["fw_version"] | "-";
    info.repo = doc["info"]["repo"] | "github.com/paclema";


}


// Loads the configuration from a file
void WebConfigServer::loadConfigurationFile(const char *filename){
  // Open file for reading
  File file = SPIFFS.open(filename, "r");

  // Allocate a temporary JsonDocument
  // Don't forget to change the capacity to match your requirements.
  // Use arduinojson.org/v6/assistant to compute the capacity.
  DynamicJsonDocument doc(CONFIG_JSON_SIZE);

  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, file);
  if (error)
    Serial.println(F("Failed to read file, using default configuration"));

  // Parse file to Config struct object:
  parseConfig(doc);

  // Close the file (Curiously, File's destructor doesn't close the file)
  file.close();
}


// Saves the Config struct configuration to a file
void WebConfigServer::saveConfigurationFile(const char *filename){
  // Delete existing file, otherwise the configuration is appended to the file
  SPIFFS.remove(filename);

  // Open file for writing
  // File file = SD.open(filename, FILE_WRITE);
  File file = SPIFFS.open(filename, "w");
  if (!file) {
    Serial.println(F("Failed to create file"));
    return;
  }

  // Allocate a temporary JsonDocument
  // Don't forget to change the capacity to match your requirements.
  // Use arduinojson.org/assistant to compute the capacity.
  DynamicJsonDocument doc(CONFIG_JSON_SIZE);

  // Network object:
  doc["network"]["ssid_name"] = network.ssid_name;
  doc["network"]["ssid_password"] = network.ssid_password;

  // Network object:
  doc["mqtt"]["server"] = mqtt.server;
  doc["mqtt"]["port"] = mqtt.port;

  // Services object:
  doc["services"]["FTP"] = mqtt.server;
  doc["services"]["port"] = mqtt.port;

  // Serialize JSON to file
  if (serializeJson(doc, file) == 0) {
    Serial.println(F("Failed to write to file"));
  }

  // Close the file
  file.close();
}


// Prints the content of a file to the Serial
void WebConfigServer::printFile(String filename){
  // Open file for reading
  File file = SPIFFS.open(filename, "r");
  if (!file) {
    Serial.println(F("Failed to read file"));
    return;
  }

  // Extract each characters by one by one
  while (file.available()) {
    Serial.print((char)file.read());
  }
  Serial.println();

  // Close the file
  file.close();
}


// Restore the backup of a file:
void WebConfigServer::restoreBackupFile(String filenamechar){

      String filename = filenamechar;
      if(!filename.startsWith("/")) filename = "/"+filename;
      String filename_bak =  "/.bak"+filename;
      Serial.print("Restoring backup for: "); Serial.println(filename);
      // Delete existing file, otherwise the configuration is appended to the file
      SPIFFS.remove(filename);

      File file = SPIFFS.open(filename, "w+");
      if (!file) {
        Serial.print(F("Failed to read file: "));Serial.println(filename);
        return;
      }

      //Serial.print("Opened: "); Serial.println(filename);
      File file_bak = SPIFFS.open(filename_bak, "r");
      if (!file) {
        Serial.print(F("Failed to read backup file: "));Serial.println(filename_bak);
        return;
      }

      //Serial.print("Opened: "); Serial.println(filename_bak);

      size_t n;
      uint8_t buf[64];
      while ((n = file_bak.read(buf, sizeof(buf))) > 0) {
        file.write(buf, n);
      }
      Serial.println("Backup restored");
      file_bak.close();
      file.close();

      // Serial.println("New config:");
      //printFile(filename);
}


void WebConfigServer::updateGpio(ESP8266WebServer *server){
  String gpio = server->arg("id");
  String val = server->arg("val");
  String success = "1";

  int pin = D5;
  if ( gpio == "D5" ) {
    pin = D5;
  } else if ( gpio == "D7" ) {
     pin = D7;
   } else if ( gpio == "D8" ) {
     pin = D8;
   } else if ( gpio == "LED_BUILTIN" ) {
     pin = LED_BUILTIN;
   } else {
     // Built-in nodemcu GPI16, pin 16 led D0
     // esp12 led is 2 or D4
     pin = LED_BUILTIN;

    }

  // Reverse current LED status:
  pinMode(pin, OUTPUT);
  // digitalWrite(pin, LOW);
  Serial.println(pin);
  Serial.print("Current status:");
  Serial.println(digitalRead(pin));
  digitalWrite(pin, !digitalRead(pin));


  // if ( val == "true" ) {
  //   digitalWrite(pin, HIGH);
  // } else if ( val == "false" ) {
  //   digitalWrite(pin, LOW);
  // } else {
  //   success = "true";
  //   Serial.println("Err parsing GPIO Value");
  // }

  String json = "{\"gpio\":\"" + String(gpio) + "\",";
  json += "\"val\":\"" + String(val) + "\",";
  json += "\"success\":\"" + String(success) + "\"}";

  server->send(200, "application/json", json);
  Serial.println("GPIO updated!");

}


void WebConfigServer::configureServer(ESP8266WebServer *server){

  // Create configuration file
  //saveConfigurationFile(CONFIG_FILE);

  //printFile(CONFIG_FILE);

  //SERVER INIT
  //list directory
  server->serveStatic("/img", SPIFFS, "/img");
  server->serveStatic("/", SPIFFS, "/index.html");
  server->serveStatic("/css", SPIFFS, "/css");
  server->serveStatic("/js", SPIFFS, "/js");
  server->serveStatic("/certs", SPIFFS, "/certs");
  server->serveStatic("/config.json", SPIFFS, "/config.json");

  server->on("/gpio", HTTP_POST, [& ,server](){
    updateGpio(server);
  });

  server->on("/save_config", HTTP_POST, [& ,server](){

    DynamicJsonDocument doc(CONFIG_JSON_SIZE);
    deserializeJson(doc, server->arg("plain"));

    // JsonObject network = doc["network"];

    Serial.print("JSON POST: ");
    serializeJsonPretty(doc, Serial);
    Serial.println("");

    // Parse file to Config struct object to update internal config:
    parseConfig(doc);

    // Save the config file with new configuration:
    saveWebConfigurationFile(CONFIG_FILE,doc);

    server->send ( 200, "text/json", "{success:true}" );

  });

  server->on("/restore_config", HTTP_POST, [& ,server](){
    // StaticJsonBuffer<200> newBuffer;
    // JsonObject& newjson = newBuffer.parseObject(server->arg("plain"));
    //
    // server->send ( 200, "text/json", "{success:true}" );
/*
    StaticJsonDocument doc(CONFIG_JSON_SIZE);
    //DynamicJsonDocument doc(CONFIG_JSON_SIZE);
    deserializeJson(doc, server->arg("plain"));

    // JsonObject network = doc["network"];

    Serial.print("JSON POST: ");
    serializeJsonPretty(doc, Serial);
    Serial.println("");

    // Restore the filename requested:
    restoreBackupFile(doc["filename"]);

    // server->send ( 200, "text/json", "{success:true}" );
*/


    // Serial.print("JSON POST: "); Serial.println(server->arg("plain"));
    // Serial.print("JSON POST argName: "); Serial.println(server->argName(0));
    // Serial.print("JSON POST args: "); Serial.println(server->args());

      if (server->args() > 0){
         for (int i = 0; i < server->args(); i++ ) {
             // Serial.print("POST Arguments: " ); Serial.println(server->args(i));
             Serial.print("Name: "); Serial.println(server->argName(i));
             Serial.print("Value: "); Serial.println(server->arg(i));
        }
     }

    if( ! server->hasArg("filename") || server->arg("filename") == NULL){
      server->send(400, "text/plain", "400: Invalid Request");
    } else{
      Serial.print("File to restore: "); Serial.println(server->arg("filename"));
      restoreBackupFile(server->arg("filename"));
      server->send ( 200, "text/json", "{success:true}" );
    }


  });

  server->on("/restart", HTTP_POST, [& ,server](){
    // if (!handleFileRead(server.uri())) {
      // server->send(404, "text/plain", "FileNotFound");
    // }
    if( ! server->hasArg("restart") || server->arg("restart") == NULL){
      server->send(400, "text/plain", "400: Invalid Request");
    } else{
      Serial.print("File to restore: "); Serial.println(server->arg("restart"));
      if (server->arg("restart") == "true"){
        server->send ( 200, "text/json", "{success:true}" );


        server->close();
        server->stop();
        WiFi.disconnect();
        SPIFFS.end();
        ESP.restart();
      }
      server->send ( 200, "text/json", "{success:false}" );
    }

  });


  //called when the url is not defined here
  //use it to load content from SPIFFS
  server->onNotFound([& ,server]() {
    // if (!handleFileRead(server.uri())) {
      server->send(404, "text/plain", "FileNotFound");
    // }
  });

  server->begin();
  Serial.println ( "HTTP server started" );


}


void WebConfigServer::handle(void){

}
