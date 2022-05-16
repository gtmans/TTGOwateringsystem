// red sensor2 ESPNOW
// for sensortype is LilyGO TTGO T-Higrow ESP32 - DHT12 Sensor
// In Arduino IDE: Boards ESP32 Arduino / ESP32 Dev Module
// https://randomnerdtutorials.com/esp8266-esp-now-wi-fi-web-server/

#include <esp_now.h>
#include <esp_wifi.h>
#include "WiFi.h"
#include "EEPROM.h"
bool measure=true;

// REPLACE WITH RECEIVER MAC Address
//MAC Address of the receiver 
//uint8_t broadcastAddress[] = {0xE8, 0xDB, 0x84  0xDC  0x31, 0x40}; // 8266 D1-MINI headers
//uint8_t broadcastAddress[] = {0xE8, 0xDB, 0x84  0xDA  0xDD, 0xC9}; // 8266 D1-MINI los
//uint8_t broadcastAddress[] = {0x84, 0xF7, 0x03  0xD8  0x06, 0x76}; // 8266 D1-S2
//uint8_t broadcastAddress[] = {0x98, 0xCD, 0xAC, 0x13, 0xC8, 0x7A}; // 8266 TTGO 0.9 kastje
uint8_t broadcastAddress[] = {0x98, 0xCD, 0xAC, 0x13, 0xD7, 0x1C}; // 8266 TTGO 0.9 los 2 knop
//uint8_t broadcastAddress[] = {0x94, 0xB9, 0x7E, 0xD3, 0xE5, 0x58}; // 32   T-DISPLAY ROOD TTGO-LORA32
//uint8_t broadcastAddress[] = {0x80, 0x7D, 0x3A, 0xE6, 0x79, 0x6C}; // 32   BME sensor
//uint8_t broadcastAddress[] = {0x30, 0x83, 0x98, 0xE2, 0xBE, 0xCC}; // 32   M5STICK-CPLUS

int dry=3500; // value for dry sensor
int wet=1734; // value for wet sensor

// Structure example to send data
// Must match the receiver structure

typedef struct struct_message {
  int           id;
  float         temp;
  float         hum;
  unsigned int  readingId;
} struct_message;

// Create a struct_message called myData
struct_message myData;

unsigned int readingId = 0;
#define BOARD_ID 2

//esp_now_peer_info_t peerInfo;

// Insert your SSID
constexpr char WIFI_SSID[] = "mySSID";

int32_t getWiFiChannel(const char *ssid) {
  if (int32_t n = WiFi.scanNetworks()) {
      for (uint8_t i=0; i<n; i++) {
          if (!strcmp(ssid, WiFi.SSID(i).c_str())) {
              return WiFi.channel(i);
          }
      }
  }
  return 0;
}


int i=0;
bool success = false;

unsigned long lastTime = 0;  
unsigned long timerDelay = 3000;  // send readings timer

// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
//  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");

  if   (status == ESP_NOW_SEND_SUCCESS) {
    Serial.println("Delivery Success");
    success=true;} 
  else {
    Serial.println("Delivery Failed trying again in 5 seconds");
    success=false;} 
    delay(5000);

  }

#include "ESPAsyncWebServer.h"
#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#include <Wire.h>
//#include <Adafruit_Sensor.h>
//#include <Adafruit_BME280.h>

#include "DHT.h";
#define DHTPIN 16
#define DHTTYPE DHT11
// Initialize DHT sensor for normal 16mhz Arduino
DHT dht(DHTPIN, DHTTYPE);
float hum;              // Stores humidity value in percent
float temp;             // Stores temperature value in Celcius

#include "gewoon_secrets.h"
/*
// gewoon_secrets.h: wifi passwords and weather.api get yours at api.openweathermap.org
const char* ssid     = "mySSID";        
const char* password = "myWIFIpassword";
String town="Apeldoorn";//weather api           
String Country="NL";               
const String key = "095e719fe1a290c29b29bbb364326bcd";
*/

// from bme280HIGROWtest
#define SOIL_PIN                (32)
#define LED_PIN                 (16)
#define OB_BH1750_ADDRESS       (0x23)//lightmeter
#define BME_SCK 13
#define BME_MISO 12
#define BME_MOSI 11
#define BME_CS 10
#define TEMP_CORR 5
//Adafruit_BME280 bme; // I2C

int               sleepmins     = 70;
int               sleeptime     = sleepmins * 60; // 1 mins * 60 seconds

void setup(){
  Serial.begin    (115200);
  EEPROM.begin    (8);
  readingId =     EEPROM.readInt  (0); 
  if (readingId>=32767){readingId=0;}
  unsigned long     sleepingtime  = (sleeptime * uS_TO_S_FACTOR);
  esp_sleep_enable_timer_wakeup (sleepingtime);
  delay           (1000);
  Serial.println  (__FILE__);//name of this doc  

if (measure){
  Serial.println("20 seconds to measure power");
  delay(20000);
  Serial.println("starting DHT ...");
}
  //tinycode
  pinMode         (4, OUTPUT);
  digitalWrite    (4, HIGH);
  delay           (200);  
  Wire.begin      (25, 26);
/*
  unsigned        status;
  status = bme.begin(0x77, &Wire);
  if (!status) {
      Serial.println  ("Could not find a valid BME280 sensor, check wiring, address, sensor ID!");
      Serial.print    ("SensorID was: 0x"); 
      Serial.println  (bme.sensorID(),16);
      Serial.print    ("        ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n");
      Serial.print    ("   ID of 0x56-0x58 represents a BMP 280,\n");
      Serial.print    ("        ID of 0x60 represents a BME 280.\n");
      Serial.print    ("        ID of 0x61 represents a BME 680.\n");
      while (1) delay (10);
  }
*/  
    // Start DHT Sensor
  dht.begin(); 
if (measure){
  Serial.println("20 seconds to measure power");
  delay(20000);
  Serial.println("starting WIFI ...");
}

  // Set device as a Wi-Fi Station and set channel
  WiFi.mode(WIFI_STA);
  int32_t channel = getWiFiChannel(WIFI_SSID);
  WiFi.printDiag(Serial); // Uncomment to verify channel number before
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);
  WiFi.printDiag(Serial); // Uncomment to verify channel change after
  //Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);
  
  //Register peer
  esp_now_peer_info_t peerInfo;
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.encrypt = false;
  
  //Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
  if (measure){
    Serial.println("20 seconds to measure power");
    delay(20000);
    Serial.println("starting loop");
  }
}
 
void loop(){
//    hum = dht.readHumidity();       // Get Humidity value
//    temp= dht.readTemperature();    // Get Temperature value
//    if (isnan(hum) || isnan(temp) ) { // Check if any reads failed and exit early (to try again).
//        Serial.println(F("Failed to read from DHT sensor!"));
//    return;
   
   // Set values to send
    int humi         = analogRead(SOIL_PIN);
        Serial.println(humi);
    myData.id        = BOARD_ID;
//    myData.temp      = bme.readTemperature();
//    myData.hum       = map(humi,wet,dry,100,0);
    myData.temp      = dht.readTemperature();
    myData.hum       = map(humi,wet,dry,100,0);
    myData.readingId = readingId++;
 
    // Send message via ESP-NOW
   //  success = false;
int tries = 0;
    while (success==false){
          esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
          Serial.print      ("sending message ");
          Serial.print      (readingId);
          Serial.print      (" ...");
          Serial.print      (myData.hum);
          Serial.println    ("%");
          tries++;
          if (tries==3)    {break;}
          delay             (5000);
    }

EEPROM.writeInt     (0,readingId); 
EEPROM.commit(); 
digitalWrite        (SOIL_PIN, LOW);  
digitalWrite        (4, LOW); //turn off GPIO16 
Serial.println      ("Going to sleep for  " + String(sleeptime/60) + " Minutes");
delay               (200);

esp_deep_sleep_start();

}
