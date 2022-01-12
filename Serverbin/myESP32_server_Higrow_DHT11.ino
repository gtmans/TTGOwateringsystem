//witte sensor1 COM12 static IP 192.168.2.10
// sensortype is LilyGO TTGO T-Higrow ESP32 - DHT11
// In Arduino IDE: Boards ESP32 Arduino / ESP32 Dev Module
#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */

//from esp8266OneWireDHT12-example
#include <Wire.h>
#include <DHT12.h>
#include "Arduino.h"
#include "secrets.h"
/*
// secrets.h: wifi passwords and weather.api get yours at api.openweathermap.org
const char* ssid     = "mySSID";        
const char* password = "myWIFIpassword";
String town="Apeldoorn";//weather api           
String Country="NL";               
const String key = "095e789fe1a290c29b29bbb364346bcd";
*/

// from bme280HIGROWtest
#define SOIL_PIN                (32)
#define LED_PIN                 (16)
#define DHT_PIN                 (22)
#define TEMP_CORR 5

DHT12 dht12(LED_PIN, true);
int               sleepmins     = 70;
int               sleeptime     = sleepmins * 60; // 70 mins * 60 seconds

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

String readTemp()   {return String(dht12.readTemperature()-TEMP_CORR);}
String readHumi()   {return String(dht12.readHumidity());}
String readSoilhum(){return String(map(analogRead(SOIL_PIN), 0, 4095, 100, 0));}  
String waiting4()   {return String(sleepmins);}

void setup(){
  Serial.begin      (115200);
  unsigned long     sleepingtime  = (sleeptime * uS_TO_S_FACTOR);
  esp_sleep_enable_timer_wakeup (sleepingtime);
  delay             (1000);
  Serial.println    (__FILE__);//name of this doc 
  pinMode           (4, OUTPUT);
  digitalWrite      (4, HIGH); //turn on GPIO16 
  pinMode           (SOIL_PIN,INPUT);
  digitalWrite      (SOIL_PIN,HIGH);  
  pinMode           (DHT_PIN, INPUT);
  digitalWrite      (DHT_PIN, HIGH);  
  pinMode           (LED_PIN, OUTPUT);
  digitalWrite      (LED_PIN, HIGH); //turn a led 
  delay             (200);
  
  dht12.begin();
  
  // Set your Static IP address
  IPAddress local_IP(192, 168, 2, 10);
  // Set your Gateway IP address
  IPAddress gateway(192, 168, 2, 254);
  IPAddress subnet(255, 255, 255, 0);
  IPAddress primaryDNS(8, 8, 8, 8);   //optional
  IPAddress secondaryDNS(8, 8, 4, 4); //optional
  // Configures static IP address
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("STA Failed to configure");
  }
  
  // Connect to Wi-Fi
  WiFi.begin        (ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }
  Serial.print      ("Started webserver at ");  
  Serial.println     (WiFi.localIP());

  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", readTemp().c_str());
  });
  server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", readHumi().c_str());
  });
  server.on("/soilhum", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", readSoilhum().c_str());
  });
  server.on("/sleeptime", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", waiting4().c_str());
  });

  digitalWrite      (LED_PIN, LOW); //turn off a led 
      
  // Start server
  server.begin();
}
 
void loop(){
Serial.println      ("Allowing web access for 2 minutes");  
delay               (120000)       ; //wait 2 minutes
digitalWrite        (SOIL_PIN, LOW);  
digitalWrite        (DHT_PIN, LOW); 
digitalWrite        (LED_PIN, LOW); 
digitalWrite        (4, LOW); //turn off GPIO16 
Serial.println      ("Going to sleep for  " + String(sleeptime/60) + " Minutes");
delay               (200);

esp_deep_sleep_start();
}
