//https://randomnerdtutorials.com/esp8266-esp-now-wi-fi-web-server/

// ESPNOW 8266 RECEIVER (for TTGO 0.9 screen)
//
//MAC Address of the receiver 
//uint8_t broadcastAddress[] = {0xE8, 0xDB, 0x84  0xDC  0x31, 0x40}; // 8266 D1-MINI headers
//uint8_t broadcastAddress[] = {0xE8, 0xDB, 0x84  0xDA  0xDD, 0xC9}; // 8266 D1-MINI los
//uint8_t broadcastAddress[] = {0x84, 0xF7, 0x03  0xD8  0x06, 0x76}; // 8266 D1-S2
//uint8_t broadcastAddress[] = {0x98, 0xCD, 0xAC, 0x13, 0xC8, 0x7A}; // 8266 TTGO 0.9 kastje
//uint8_t broadcastAddress[] = {0x98, 0xCD, 0xAC, 0x13, 0xD7, 0x1C}; // 8266 TTGO 0.9 los
//uint8_t broadcastAddress[] = {0x94, 0xB9, 0x7E, 0xD3, 0xE5, 0x58}; // 32   T-DISPLAY ROOD TTGO-LORA32
//uint8_t broadcastAddress[] = {0x80, 0x7D, 0x3A, 0xE6, 0x79, 0x6C}; // 32   BME sensor
//uint8_t broadcastAddress[] = {0x30, 0x83, 0x98, 0xE2, 0xBE, 0xCC}; // 32   M5STICK-CPLUS

/*  
  Based on https://RandomNerdTutorials.com/esp8266-esp-now-wi-fi-web-server/ by Rui Santos
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*/

#include          <espnow.h>
#include          <ESP8266WiFi.h>
#include          "ESPAsyncWebServer.h"
#include          "ESPAsyncTCP.h"
#include          <Arduino_JSON.h>
#include          <EEPROM.h>
#define           EEPROM_SIZE 8

//from myESP8266-Client-NOW_aka-thin10
  #define         relayPin  1
  #define         lbutton   13
  #define         rbutton   15  
  #define         OLED_SDA  2
  #define         OLED_SCL  14
  #define         OLED_RST  4
  #include        <U8g2lib.h> 
  U8G2_SSD1306_128X32_UNIVISION_F_SW_I2C u8g2(U8G2_R0, OLED_SCL, OLED_SDA , OLED_RST);
  #define         mono 1

  int     sensor;
  int     TMinutes;
  int     threshold;                      // = 25; from eeprom when dryer water!
  int     oldthreshold;
  int     humidity     =  99;
  int     humidity1    =  0;
  int     humidity2    =  0;
  int     s1online     =  0;              //times online
  int     s2online     =  0;
  bool    buttons;                        // are there buttons attached to set threshold?
  bool    dispset     = false;            // used 4 display setup some time during loop (20%)
  bool    water       = false;            // do we water?
  bool    oldwater    = false;            // previous water
  String  SoilValue[3] =  {"","11","33"}; //SoilValue[1]= sensor1 and SoilValue[2]=reading sensor2
  int     SoilHum  [3] =  {0,99,99};
  int     PMinutes [3] =  {0,0,0};        // passed minutes counter
  int     Misses   [3] =  {0,0,0};        // missed messages counter

#include "gewoon_secrets.h" //instead of the two below constants
//Replace with your network credentials (STATION)
//const char* ssid = "REPLACE_WITH_YOUR_SSID";
//const char* password = "REPLACE_WITH_YOUR_PASSWORD";

// Structure example to receive data
// Must match the sender structure
typedef struct struct_message {
  int           id;
  float         temp;
  float         hum;
  unsigned int  readingId;
} struct_message;
struct_message incomingReadings;

JSONVar board;

AsyncWebServer server   (80);
AsyncEventSource events ("/events");

// callback function that will be executed when data is received
void OnDataRecv   (uint8_t * mac_addr, uint8_t *incomingData, uint8_t len) { 
  // Copies the sender mac address to a string
  char macStr[18];
  Serial.print    ("Packet received from: ");
  snprintf        (macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
                   mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.println  (macStr);
  memcpy          (&incomingReadings, incomingData, sizeof(incomingReadings));
  
  board["id"]         = incomingReadings.id;
  board["temperature"]= incomingReadings.temp;
  board["humidity"]   = incomingReadings.hum;
  board["readingId"]  = String(incomingReadings.readingId);
  String jsonString   = JSON.stringify(board);
  events.send(jsonString.c_str(), "new_readings", millis());
  Serial.printf       ("Board ID %u: %u bytes\n", incomingReadings.id, len);
  Serial.printf       ("t value: %4.2f \n", incomingReadings.temp);
  Serial.printf       ("h value: %4.2f \n", incomingReadings.hum);
  Serial.printf       ("readingID value: %d \n", incomingReadings.readingId);
  Serial.println      ();

//process things
  sensor            = incomingReadings.id;
  int minpassed     = PMinutes[sensor];
  int misses        = 0;
  if (minpassed>90) { misses = minpassed/70;} 
  Misses[sensor]    = Misses[sensor] + misses;
  PMinutes[sensor]  = 0;
  SoilHum [sensor]  = incomingReadings.hum; // store value
  humidity          = SoilHum[1];
  if (SoilHum[2]<humidity){humidity=SoilHum[2];}
  if (humidity<=threshold){water=true;}else{water=false;}
  Serial.print        ("Received message from sensor ");
  Serial.print        (sensor);
  Serial.print        (" humidity:");
  Serial.print        (humidity);
  Serial.print        (" last interval ");
  Serial.print        (minpassed);
  Serial.print        (" minutes. Total misses:");
  Serial.print        (Misses[sensor]);
  if (water){Serial.println("WATER");} else {Serial.println("DO NOT WATER");}
  DispScreen();
}

void DispScreen(){
    u8g2.clearBuffer  ();
    u8g2.setFont      (u8g2_font_logisoso32_tf);   // 32h
    u8g2.setCursor    (0,32);
    u8g2.print        (humidity);    
    u8g2.setFont      (u8g2_font_courB10_tf);   // 16h
    u8g2.setCursor    (46,16);
    u8g2.print        (SoilHum[1]); 
    u8g2.print        ("% ");
    u8g2.setCursor    (80,16);
    u8g2.print        (PMinutes[1]); 
    u8g2.print        (" "); 
    u8g2.print        (threshold);
    u8g2.setCursor    (46,32);
    u8g2.print        (SoilHum[2]); 
    u8g2.print        ("% ");
    u8g2.setCursor    (80,32);
    u8g2.print        (PMinutes[2]); 
    u8g2.print        (" "); 
    u8g2.print        (humidity); 
    u8g2.sendBuffer   ();
}

void   checkbuttons(){
#if defined(mono)                       // 0.9 monochrome   
  u8g2.setFont(u8g2_font_courB24_tf);   // bigger font
  bool pressed = true;
#else
  bool pressed = false;
#endif

    if (digitalRead (lbutton)==pressed){           // left
      Serial.println("left button pressed");
      if(threshold>0){threshold--;}
      displaysetup     ();
      delay(250);
    }
    if (digitalRead (rbutton)==pressed){           // right
      Serial.println("right button pressed");   
      if(threshold<99){threshold++;}
      displaysetup     ();
      delay(250);
    }
    if (threshold!=oldthreshold){
      EEPROM.begin    (EEPROM_SIZE);
      EEPROM.write    (0,threshold); 
      EEPROM.commit   ();
      oldthreshold =  threshold;
      if (humidity<=threshold){water=true;}else{water=false;}
    }
}

void displaysetup(){     // display setup threshold
    u8g2.clearBuffer  ();
    u8g2.setFont      (u8g2_font_open_iconic_all_4x_t);//32 pix high
    u8g2.setCursor    (0,32);
    u8g2.print        (char(129));//setup 
    u8g2.setCursor    (32+4,32);
    u8g2.setFont      (u8g2_font_courB24_tf);       
    u8g2.print        (threshold);
    u8g2.print        ("%");
    
    u8g2.setFont      (u8g2_font_open_iconic_all_4x_t);//32 pix high
    u8g2.setCursor    (96,32);
    if (water)        {u8g2.print(char(152));}//drop    
//    u8g2.print(char(152));//drop    
    u8g2.sendBuffer   (); 
}

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>MANSFILMT DASHBOARD</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
  <link rel="icon" href="data:,">
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    h1 {  font-size: 2rem;}
    body {  margin: 0;}
    .topnav { overflow: hidden; background-color: #2f4468; color: white; font-size: 1.7rem; }
    .content { padding: 20px; }
    .card { background-color: white; box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5); }
    .cards { max-width: 700px; margin: 0 auto; display: grid; grid-gap: 2rem; grid-template-columns: repeat(auto-fit, minmax(320px, 1fr)); }
    .reading { font-size: 2.8rem; }
    .timestamp { color: #bebebe; font-size: 1rem; }
    .card-title{ font-size: 1.2rem; font-weight : bold; }
    .card.temperature { color: #B10F2E; }
    .card.humidity { color: #50B8B4; }
  </style>
</head>
<body>
  <div class="topnav">
    <h1>ESP-NOW DASHBOARD</h1>
  </div>
  <div class="content">
    <div class="cards">
      <div class="card temperature">
        <p class="card-title"><i class="fas fa-thermometer-half"></i> voortuin - temp</p><p><span class="reading"><span id="t1"></span> &deg;C</span></p><p class="timestamp">Laatse data: <span id="rt1"></span></p>
      </div>
      <div class="card humidity">
        <p class="card-title"><i class="fas fa-tint"></i> voortuin - vocht</p><p><span class="reading"><span id="h1"></span> &percnt;</span></p><p class="timestamp">Laatse data: <span id="rh1"></span></p>
      </div>
      <div class="card temperature">
        <p class="card-title"><i class="fas fa-thermometer-half"></i> achtertuin - temp</p><p><span class="reading"><span id="t2"></span> &deg;C</span></p><p class="timestamp">Laatse data: <span id="rt2"></span></p>
      </div>
      <div class="card humidity">
        <p class="card-title"><i class="fas fa-tint"></i> achtertuin - vocht</p><p><span class="reading"><span id="h2"></span> &percnt;</span></p><p class="timestamp">Laatse data: <span id="rh2"></span></p>
      </div>
    </div>
  </div>
<script>
function getDateTime() {
  var currentdate = new Date();
  var datetime = currentdate.getDate() + "/"
  + (currentdate.getMonth()+1) + "/"
  + currentdate.getFullYear() + " om "
  + currentdate.getHours() + ":"
  + currentdate.getMinutes() + ":"
  + currentdate.getSeconds();
  return datetime;
}
if (!!window.EventSource) {
 var source = new EventSource('/events');
 
 source.addEventListener('open', function(e) {
  console.log("Events Connected");
 }, false);
 source.addEventListener('error', function(e) {
  if (e.target.readyState != EventSource.OPEN) {
    console.log("Events Disconnected");
  }
 }, false);
 
 source.addEventListener('message', function(e) {
  console.log("message", e.data);
 }, false);
 
 source.addEventListener('new_readings', function(e) {
  console.log("new_readings", e.data);
  var obj = JSON.parse(e.data);
  document.getElementById("t"+obj.id).innerHTML = obj.temperature.toFixed(2);
  document.getElementById("h"+obj.id).innerHTML = obj.humidity.toFixed(2);
  document.getElementById("rt"+obj.id).innerHTML = getDateTime();
  document.getElementById("rh"+obj.id).innerHTML = getDateTime();
 }, false);
}
</script>
</body>
</html>)rawliteral";

void setup() {
  Serial.begin(115200);
  EEPROM.begin      (EEPROM_SIZE);
  threshold     =   EEPROM.read(0);
  if (threshold>75){threshold=35;}
  oldthreshold  =   threshold;

  u8g2.begin        ();
  u8g2.setContrast  (3);                  // x from 0 to 255 works, but do not disable oled. value x=0 still display some data..
  u8g2.clearBuffer  ();
  Serial.println    (__FILE__);         //name of this doc 
  
  IPAddress local_IP(192, 168, 2, 9);   // Set your Static IP address
  IPAddress gateway(192, 168, 2, 254);  // Set your Gateway IP address
  IPAddress subnet(255, 255, 255, 0);
  IPAddress primaryDNS(8, 8, 8, 8);     //optional
  IPAddress secondaryDNS(8, 8, 4, 4);   //optional
  // Configures static IP address
  if (!WiFi.config    (local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
     Serial.println   ("STA Failed to configure");
  } else {Serial.print("IP address set to ");Serial.println(local_IP);}

  // Set the device as a Station and Soft Access Point simultaneously
  WiFi.mode(WIFI_AP_STA);  
  // Set device as a Wi-Fi Station
  WiFi.begin(ssid, password);

  u8g2.clearBuffer  ();
  u8g2.setFont      (u8g2_font_courB10_tf);   
  u8g2.setCursor    (0,16);
  u8g2.print        ("Setting WIFI");
  u8g2.sendBuffer   ();
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println  ("Setting as a Wi-Fi Station..");
    u8g2.print      (".");
    u8g2.sendBuffer ();
  }
  Serial.print      ("Station IP Address: ");
  Serial.println    (WiFi.localIP());
  Serial.print      ("Wi-Fi Channel: ");
  Serial.println    (WiFi.channel());

  u8g2.clearBuffer  ();
  u8g2.setFont      (u8g2_font_courB10_tf);   // 16h
  u8g2.setCursor    (0,16);
  u8g2.print        ("IP:");
  u8g2.print        (WiFi.localIP()); 
  u8g2.setCursor    (0,32);
  u8g2.print        ("Kanaal:");
  u8g2.print        (WiFi.channel()); 
  u8g2.sendBuffer   ();

  // Init ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  // Once ESPNow is successfully Init, we will register for recv CB to get recv packer info

  esp_now_register_recv_cb(OnDataRecv);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });
   
  events.onConnect([](AsyncEventSourceClient *client){
    if(client->lastId()){
      Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }
    // send event with message "hello!", id current millis
    // and set reconnect delay to 1 second
    client->send("hello!", NULL, millis(), 10000);
  });
  server.addHandler(&events);
  server.begin();

  pinMode           (lbutton, INPUT);
  pinMode           (rbutton, INPUT);
  pinMode           (relayPin,OUTPUT);
  digitalWrite      (relayPin,LOW);
  if (digitalRead   (lbutton)==true){buttons=false;} else {buttons=true;}// no button attached on 8266

}
 
void loop() {
  static unsigned long lastEventTime = millis();
  static const unsigned long EVENT_INTERVAL_MS = 60000;
  if ((millis() - lastEventTime) > EVENT_INTERVAL_MS) {
    PMinutes[1]++;
    PMinutes[2]++;
    DispScreen();
    events.send("ping",NULL,millis());
    lastEventTime = millis();
    dispset=false;
    if (water && !oldwater){        // water has become true turn on relay
      digitalWrite(relayPin,HIGH);
      oldwater=true;    
    }   
    if (!water && oldwater){        // water has become false turn off relay
      digitalWrite(relayPin,LOW);
      oldwater=false;    
    }   
}

  if ((millis() - lastEventTime) > .5 * EVENT_INTERVAL_MS) {
    if (!dispset) {dispset=true;displaysetup();}
  }
  
  if (buttons){checkbuttons();} // only read port when buttons are attached
}
