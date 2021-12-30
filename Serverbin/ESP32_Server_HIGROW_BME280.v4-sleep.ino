/*rode sensor1 static IP 11
  Rui Santos Complete project details at 
  https://RandomNerdTutorials.com/esp32-client-server-wi-fi/
*/
#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include "gewoon_secrets.h"

// from bme280HIGROWtest
#define SOIL_PIN                (32)
#define OB_BH1750_ADDRESS       (0x23)//lightmeter
#define BME_SCK 13
#define BME_MISO 12
#define BME_MOSI 11
#define BME_CS 10
#define TEMP_CORR 5

Adafruit_BME280 bme; // I2C
int               sleepmins     = 70;
int               sleeptime     = sleepmins * 60; // 70 mins * 60 seconds

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

String readTemp()   {return String(bme.readTemperature()-TEMP_CORR);}
String readHumi()   {return String(bme.readHumidity());}
String readPres()   {return String(bme.readPressure() / 100.0F);}
String readSoilhum(){return String(map(analogRead(SOIL_PIN), 0, 4095, 100, 0));  
}

void setup(){
  Serial.begin    (115200);
  unsigned long     sleepingtime  = (sleeptime * uS_TO_S_FACTOR);
  esp_sleep_enable_timer_wakeup (sleepingtime);
  delay(1000);
  Serial.println    (__FILE__);//name of this doc  
  //tinycode
  pinMode         (4, OUTPUT);
  digitalWrite    (4, HIGH);
  delay           (200);
  Wire.begin (25, 26);
  unsigned status;
 
  status = bme.begin(0x77, &Wire);
  if (!status) {
      Serial.println("Could not find a valid BME280 sensor, check wiring, address, sensor ID!");
      Serial.print("SensorID was: 0x"); Serial.println(bme.sensorID(),16);
      Serial.print("        ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n");
      Serial.print("   ID of 0x56-0x58 represents a BMP 280,\n");
      Serial.print("        ID of 0x60 represents a BME 280.\n");
      Serial.print("        ID of 0x61 represents a BME 680.\n");
      while (1) delay(10);
  }

  
  // Set your Static IP address
IPAddress local_IP(192, 168, 2, 11);
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
  WiFi.begin          (ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }
  Serial.print      ("Started webserver at ");  
  Serial.println    (WiFi.localIP());

  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", readTemp().c_str());
  });
  server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", readHumi().c_str());
  });
  server.on("/pressure", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", readPres().c_str());
  });
  server.on("/soilhum", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", readSoilhum().c_str());
  });
    
  // Start server
  server.begin();
}
 
void loop(){
  
Serial.println      ("Allowing web access for 2 minutes");  
delay               (120000)       ; //wait 2 minutes
digitalWrite        (SOIL_PIN, LOW);  
//digitalWrite        (DHT_PIN, LOW);  
digitalWrite        (4, LOW); //turn off GPIO16 
Serial.println      ("Going to sleep for  " + String(sleeptime/60) + " Minutes");
delay               (200);

esp_deep_sleep_start();
  }
