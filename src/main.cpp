#include <Arduino.h>

#define DEBUG_ESP_CORE

// Used for light sleep
extern "C" {
  #include "user_interface.h"
}

#include <ESP8266WiFi.h>


// MQTT
#include <PubSubClient.h>
WiFiClientSecure wifiClientSecure;    // To use with mqtt and certificates
WiFiClient wifiClient;                // To use with mqtt without certificates
PubSubClient mqttClient;
long connection_time = millis();

// Configuration
#include "WebConfigServer.h"
#include <ESP8266WebServer.h>
WebConfigServer config;   // <- global configuration object
ESP8266WebServer server(80);

// FTP server
#include <ESP8266FtpServer.h>
#include <FS.h>
FtpServer ftpSrv;

// OTA Includes
#include "WrapperOTA.h"
// #include <display2.h>
WrapperOTA ota;

// Reed Switch
const int reedSwitch = D1;
bool reedSwitchVal = false;
bool reedSwitchVal_last = false;



void networkRestart(void){
  if(config.status() == CONFIG_LOADED){
    // Config loaded correctly
    if (config.network.ssid_name!=NULL && config.network.ssid_password!=NULL){
        // Connect to Wi-Fi
        WiFi.mode(WIFI_STA);
        WiFi.begin(config.network.ssid_name, config.network.ssid_password);
        Serial.print("Connecting to ");Serial.print(config.network.ssid_name);
        while (WiFi.status() != WL_CONNECTED) {
          delay(300);
          Serial.print(".");
        }
        Serial.println("");
    }
  }

  // Print Local IP Address
  Serial.println(WiFi.localIP());

}


void enableServices(void){
  Serial.println("--- Services: ");

  if (config.services.OTA){
    // ota.init(&display);
    ota.init(&config);
    Serial.println("   - OTA -> enabled");
  } else Serial.println("   - OTA -> disabled");

  if (config.services.ftp.enabled && config.services.ftp.user !=NULL && config.services.ftp.password !=NULL){
    ftpSrv.begin(config.services.ftp.user,config.services.ftp.password);
    Serial.println("   - FTP -> enabled");
  } else Serial.println("   - FTP -> disabled");

  if (config.services.deep_sleep.enabled){
    // We will enable it on the loop function
    // Serial.println("   - Deep sleep -> configured");
    Serial.print("   - Deep sleep -> enabled for ");
    Serial.print(config.services.deep_sleep.sleep_time);
    Serial.print("secs after waitting ");
    Serial.print(config.services.deep_sleep.sleep_delay);
    Serial.println("secs. Choose sleep_time: 0 for infinite sleeping");
    Serial.println("     Do not forget to connect D0 to RST pin to auto-wake up! Or I will sleep forever");
  } else Serial.println("   - Deep sleep -> disabled");

  if (config.services.light_sleep.enabled){
    if (config.services.light_sleep.mode == "LIGHT_SLEEP_T")
      wifi_set_sleep_type(LIGHT_SLEEP_T);
    else if (config.services.light_sleep.mode == "NONE_SLEEP_T")
      wifi_set_sleep_type(NONE_SLEEP_T);
    else {
      Serial.println("   - Light sleep -> mode not available");
      return;
    }
    Serial.println("   - Light sleep -> enabled");
  } else Serial.println("   - Light sleep -> disabled");

  Serial.println("");

}


void callbackMQTT(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  char buff[length + 1];
  for (int i = 0; i < length; i++) {
    //Serial.print((char)payload[i]);
    buff[i] = (char)payload[i];
  }


  buff[length] = '\0';
  String message(buff);

  Serial.print(message);
  Serial.println();

/*
  if (strcmp(topic, "/lamp") == 0) {
    //Lamp color request:
    if (message.equals("red")){
      Serial.println("Turning lamp to red");
      //colorWipe(strip.Color(255, 0, 0), 10);
    }
    else if (strcmp(buff, "blue") == 0){
        Serial.println("Turning lamp to blue");
        //colorWipe(strip.Color(0, 0, 255), 10);
    } else if (message.equals("green")){
        Serial.println("Turning lamp to green");
        //colorWipe(strip.Color(0, 255, 0), 10);
    }
    //client.publish((char*)"/lamp",(char*)"color changed");
  }
*/

  Serial.print("Heap: "); Serial.println(ESP.getFreeHeap()); //Low heap can cause problems
  ///

}

void initMQTT(void){

  if (config.mqtt.enable_certificates){
    mqttClient.setClient(wifiClientSecure);
    Serial.println("Configuring MQTT using certificates");
  } else {
    mqttClient.setClient(wifiClient);
    Serial.println("Configuring MQTT without certificates");
  }

  mqttClient.setServer(config.mqtt.server.c_str(), config.mqtt.port);
  mqttClient.setCallback(callbackMQTT);

  if (config.mqtt.enable_certificates){
    // Load certificate file:
    // But you must convert it to .der
    // openssl x509 -in ./certs/IoLed_controller/client.crt -out ./certs/IoLed_controller/cert.der -outform DER
    File cert = SPIFFS.open(config.mqtt.cert_file, "r"); //replace cert.crt with your uploaded file name
    if (!cert) Serial.println("Failed to open cert file ");
    else Serial.println("Success to open cert file");

    if (wifiClientSecure.loadCertificate(cert)) Serial.println("cert loaded");
    else Serial.println("cert not loaded");
    cert.close();

    // Load private key:
    // But you must convert it to .der
    // openssl rsa -in ./certs/IoLed_controller/client.key -out ./certs/IoLed_controller/private.der -outform DER
    File private_key = SPIFFS.open(config.mqtt.key_file, "r");
    if (!private_key) Serial.println("Failed to open key file ");
    else Serial.println("Success to open key file");

    if (wifiClientSecure.loadPrivateKey(private_key)) Serial.println("key loaded");
    else Serial.println("key not loaded");
    private_key.close();

    // Load CA file:
    File ca = SPIFFS.open(config.mqtt.ca_file, "r");
    if (!ca) Serial.println("Failed to open CA file ");
    else Serial.println("Success to open CA file");

    if (wifiClientSecure.loadCACert(ca)) Serial.println("CA loaded");
    else Serial.println("CA not loaded");
    ca.close();
  }

}

void reconnectMQTT() {
  // Loop until we're reconnected
  int retries = 0;
  int max_retries = 5;
  bool mqtt_connected = false;
  while (!mqttClient.connected() && (retries <= max_retries) ) {
    Serial.print("Attempting MQTT connection...");
    if (config.mqtt.enable_user_and_pass)
      mqtt_connected = mqttClient.connect(config.mqtt.id_name.c_str(),
                                          config.mqtt.user_name.c_str(),
                                          config.mqtt.user_password.c_str());
    else
        mqtt_connected = mqttClient.connect(config.mqtt.id_name.c_str());

    if (mqtt_connected) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      String base_topic_pub = "/" + config.mqtt.id_name + "/";
      String topic_connected_pub = base_topic_pub + "connected";
      // String msg_connected = config.mqtt.id_name + " connected";
      String msg_connected ="true";
      mqttClient.publish(topic_connected_pub.c_str(), msg_connected.c_str());
      // ... and resubscribe
      String base_topic_sub = base_topic_pub + "#";
      mqttClient.subscribe(base_topic_sub.c_str());

      long time_now = millis() - connection_time;
      Serial.print("Time to setup and be connected: ");
      Serial.print(time_now/1000);
      Serial.println("s");

    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.print(" try again in 5 seconds: ");
      Serial.print(retries);
      Serial.print("/");
      Serial.println(max_retries);

      // Wait 5 seconds before retrying
      delay(5000);

    }
    retries++;
  }
}


void reconnect(void) {
  //delay(1000);
  Serial.print("--- Free heap: "); Serial.println(ESP.getFreeHeap());

  config.begin();
  networkRestart();
  config.configureServer(&server);

  // Enable services:
  enableServices();

  // Configure MQTT broker:
  initMQTT();
  reconnectMQTT();

}


void updateGPIOs(void){

  reedSwitchVal = digitalRead(reedSwitch);

  if (reedSwitchVal_last!=reedSwitchVal){
    String base_topic_pub = "/" + config.mqtt.id_name + "/";
    String topic_state_pub = base_topic_pub + "status";
    String msg;
    if (reedSwitchVal){
      Serial.println("Reed switch --> Closed");
      // msg ="false";
      msg ="closed";
    } else {
      Serial.println("Reed switch --> Open");
      // msg ="true";
      msg ="open";
    }

    reedSwitchVal_last = reedSwitchVal;
    mqttClient.publish(topic_state_pub.c_str(), msg.c_str(), true);
  }

}

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);

  // Configure pins:
  pinMode(reedSwitch, INPUT_PULLUP);

  reconnect();

  // Send the first status:
  reedSwitchVal = digitalRead(reedSwitch);
  reedSwitchVal_last = reedSwitchVal;
  String base_topic_pub = "/" + config.mqtt.id_name + "/";
  String topic_state_pub = base_topic_pub + "status";
  String msg;
  if (reedSwitchVal){
    Serial.println("Reed switch --> Closed");
    // msg ="false";
    msg ="closed";
  } else {
    Serial.println("Reed switch --> Open");
    // msg ="true";
    msg ="open";
  }
  mqttClient.publish(topic_state_pub.c_str(), msg.c_str());

  // Print some info:
  uint32_t realSize = ESP.getFlashChipRealSize();
  uint32_t ideSize = ESP.getFlashChipSize();
  FlashMode_t ideMode = ESP.getFlashChipMode();

  Serial.printf("Flash real id:   %08X\n", ESP.getFlashChipId());
  Serial.printf("Flash real size: %u bytes\n\n", realSize);

  Serial.printf("Flash ide  size: %u bytes\n", ideSize);
  Serial.printf("Flash ide speed: %u Hz\n", ESP.getFlashChipSpeed());
  Serial.printf("Flash ide mode:  %s\n", (ideMode == FM_QIO ? "QIO" : ideMode == FM_QOUT ? "QOUT" : ideMode == FM_DIO ? "DIO" : ideMode == FM_DOUT ? "DOUT" : "UNKNOWN"));

  if (ideSize != realSize) {
    Serial.println("Flash Chip configuration wrong!\n");
  } else {
    Serial.println("Flash Chip configuration ok.\n");
  }

  Serial.println("###  Looping time\n");
}

void loop() {
  // if (WiFi.status() != WL_CONNECTED) {
  //   config.begin();
  //   networkRestart();
  //   config.configureServer(&server);
  // }

  if (config.services.OTA) ota.handle();
  if (config.services.ftp.enabled) ftpSrv.handleFTP();
  server.handleClient();

  // Handle mqtt reconnection:
  if (!mqttClient.connected())
    reconnectMQTT();
  mqttClient.loop();


  // Update GPIOs:
  updateGPIOs();





  if (config.services.deep_sleep.enabled){
    // long time_now = millis();
    if (millis() > connection_time + (config.services.deep_sleep.sleep_delay*1000)){
      Serial.println("Deep sleeping...");
      if (config.services.deep_sleep.mode == "WAKE_RF_DEFAULT")
        // sleep_time is in secs, but the function gets microsecs
        ESP.deepSleep(config.services.deep_sleep.sleep_time * 1000000, WAKE_RF_DEFAULT);
      else if (config.services.deep_sleep.mode == "WAKE_RF_DISABLED")
        ESP.deepSleep(config.services.deep_sleep.sleep_time * 1000000, WAKE_RF_DISABLED);
      else if (config.services.deep_sleep.mode == "WAKE_RFCAL")
        ESP.deepSleep(config.services.deep_sleep.sleep_time * 1000000, WAKE_RFCAL);
      else if (config.services.deep_sleep.mode == "WAKE_NO_RFCAL")
        ESP.deepSleep(config.services.deep_sleep.sleep_time * 1000000, WAKE_NO_RFCAL);
      else {
        Serial.println("   - Deep sleep -> mode not available");
        return;
      }
    }
  }




}
