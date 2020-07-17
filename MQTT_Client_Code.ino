/*
Project:  thesis-fom-ss2020-mqtt_nodered
Author:   Felix Schubert
          This code is based on the code of 
          Thomas Edlinger for www.edistechlab.com .
Date:     Created 05.07.2020
Version:  V1.0
*/


#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#define wifi_ssid "wifi_id"
#define wifi_password "wifi_password"

#define mqtt_server "192.168.178.38 (MQTT-Broker Adresse)"
#define mqtt_user "broker_test"         
#define mqtt_password "broker_testpassword"     

#define humidity_topic "topiclevel1/humidity"
#define temperature_topic "topiclevel1/temperature"
#define pressure_topic "topiclevel1/pressure"

float temp = 0.0;
float hum = 0.0;
float pres = 0.0;
float diff = 1.0;

Adafruit_BME280 bme; // I2C
unsigned long delayTime;

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;


void setup() {
  Serial.begin(115200);
  Serial.println("Booting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi_ssid, wifi_password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }


   ArduinoOTA.setHostname("topiclevel1");



  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { 
      type = "filesystem";
    }

   
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  client.setServer(mqtt_server, 1883);
  
  while(!Serial);    
    unsigned status;
    status = bme.begin();  
    if (!status) {
        Serial.println("Sensor nicht richtig angeschlossen!");
        while (1);
    }
    delayTime = 1000;


}


bool checkBound(float newValue, float prevValue, float maxDiff) {
  return !isnan(newValue) &&
  (newValue < prevValue - maxDiff || newValue > prevValue + maxDiff);
}

void getValues() {
    float newPres = bme.readPressure() / 100.0F;
    float newTemp = bme.readTemperature();
    float newHum = bme.readHumidity();

    if (checkBound(newTemp, temp, diff)) {
      temp = newTemp;
      Serial.print("New Temperature:");
      Serial.println(String(temp).c_str());
      client.publish(temperature_topic, String(temp).c_str(), true);
    }

    if (checkBound(newHum, hum, diff)) {
      hum = newHum;
      Serial.print("New Huminity:");
      Serial.println(String(hum).c_str());
      client.publish(humidity_topic, String(hum).c_str(), true);
    }

    if (checkBound(newPres, pres, diff)) {
      pres = newPres;
      Serial.print("New Pressure:");
      Serial.println(String(pres).c_str());
      client.publish(pressure_topic, String(pres).c_str(), true);
    }  
}

void reconnect() {

  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "topiclevel1";
    if (client.connect(clientId.c_str(), mqtt_user, mqtt_password)) {
      Serial.println("connected");
      client.publish("outTopic", "topiclevel1 alive");
      client.subscribe("inTopic");
    } else {
        Serial.print("failed, rc=");
        Serial.print(client.state());
        Serial.println(" in 7 Sekunden nochmal versuchen!");
        delay(7000);
      }
   }
}


void loop() {
  ArduinoOTA.handle();

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;
    getValues();
  }

}
