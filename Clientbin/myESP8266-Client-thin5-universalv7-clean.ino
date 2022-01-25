/*
https://github.com/gtmans/TTGOwateringsystem 2022 by Mans Rademaker (gtmans)
for LilyGO TTGO ESP8266 with 0.91 inch OLED Display        IDE generic ESP8266
or  LilyGO TTGO T-Display V1.1 ESP32 1.14 inch TFT Display IDE ESP32   TTGO LORA32
depends on IDE setting (choice of board in IDE)
font (0.91 inch) open iconic https://github.com/olikraus/u8g2/wiki/fntgrpiconic
inspiration: http://simplestuffmatters.com/wemos-ttgo-esp8266-with-0-91-inch-oled/
inspiration: https://randomnerdtutorials.com/esp32-client-server-wi-fi/
*/
#if defined(ESP8266) //0.9 mono
  #pragma message compiling for ESP8266
  #include        <ESP8266WiFi.h>    // if ESP8266
  #include        <ESP8266Ping.h>
  #include        <ESP8266HTTPClient.h>
  #define         relayPin  1
  #define         lbutton   13
  #define         rbutton   15  
  #define         OLED_SDA  2
  #define         OLED_SCL  14
  #define         OLED_RST  4
  #include        <U8g2lib.h> 
  U8G2_SSD1306_128X32_UNIVISION_F_SW_I2C u8g2(U8G2_R0, OLED_SCL, OLED_SDA , OLED_RST);
  #define         mono 1
#elif defined(ESP32) //1.14 colordisplay
  #pragma message compiling for ESP32
  #include        <WiFi.h>           // if ESP32
  #include        <ESP32Ping.h>
  #include        <HTTPClient.h>
  #include        <TFT_eSPI.h>       // Hardware-specific library
  TFT_eSPI tft = TFT_eSPI();         // Invoke custom library
  #define         relayPin  5
  #define         portrait  0
  #define         landscape 3
  #define         lbutton   0
  #define         rbutton   35
  const int       pwmFreq           = 5000;
  const int       pwmResolution     = 8;
  const int       pwmLedChannelTFT  = 0;
  int backlight[5]                  = {10,30,60,120,220};  
#endif

#include          <Arduino.h>
#include          <SPI.h>
#include          <Wire.h>
#include          <NTPClient.h>       // https://github.com/taranais/NTPClient
#include          <WiFiUdp.h>
WiFiUDP           ntpUDP;             // Define NTP Client to get time
NTPClient         timeClient(ntpUDP); 
#include          <EEPROM.h>
#define           EEPROM_SIZE 8

int   httpResponseCode;
float avpings1  = 4;                // average #pings server1
float avpings2  = 4;                // average #pings server2
float avtime1   = 70;               // average #mins between contact server1
float avtime2   = 70;               // average #mins between contact server2
int ping1       = 0;                // #pings server1
int ping2       = 0;                // #pings server2
int oping1      = 0;                // #online pings server1
int oping2      = 0;                // #online pings server2
int totopings1  = 0;                // total of #online pings server1
int totopings2  = 0;                // total of #online pings server2
int timesinces1 = 70;               // #minutes since last seen server 1
int timesinces2 = 70;               // #minutes since last seen server 2
int OMinutes    =-1;                // total minutes since startup
int TMinutes;
int TMinutesS1;
int TMinutesS2;
int LastTMinutes= 0;
int screen      = 0;                // what screen to display
int numscreens  = 2;                // number of info screens to display -1 - extra 2 temp screens
int threshold;                      // = 25; from eeprom when dryer water!
int oldthreshold;
int humidity1   = 0;
int humidity2   = 0;
int s1online    = 0;                //times online
int s2online    = 0;
String          STHours;
String          STMinutes;
String          formattedDate;
String          dayStamp;
String          TimeStamp;
String          LastTimeStamp;
String          SoilValue[3]    =   {"","11","33"};//SoilValue[1]=reading sensor1 and SoilValue[2]=reading sensor2
const long      interval        =   15000; // check servers every 15 seconds (try 4 times a minute)
const long      interval2       =   2500;  // change display every 5 secs
unsigned long   previousMillis  =   0;
unsigned long   previousMillis2 =   0;
bool both     = false;
bool debug    = false;
bool dispping = false;
bool water    = false;        // if true:water the garden
bool oldwater = false;
bool sensor1serverup;
bool sensor2serverup;

//WIFI configuration
#include "secrets.h"
/*
// secrets.h: wifi passwords
const char* ssid     = "mySSID";        
const char* password = "myWIFIpassword";
*/
//SENSORSERVERS                 
const  IPAddress SServer1       (192,168,2,10);
const  IPAddress SServer2       (192,168,2,11);
String serverNameSoil  = "http://192.168.2.10/soilhum";
String serverName2Soil = "http://192.168.2.11/soilhum";

void setup() {
  Serial.begin      (115200);
  EEPROM.begin      (EEPROM_SIZE);
  threshold     =   EEPROM.read(0);
  if (threshold>75){threshold=35;}
  oldthreshold  =   threshold;

#if defined(mono)       //0.9 monochrome
      u8g2.begin        ();
      u8g2.setContrast(5); // x from 0 to 255 works, but do not disable oled. value x=0 still display some data..
#else
      tft.init();
      tft.setRotation   (landscape);
      tft.fillScreen    (TFT_BLACK);
      tft.setTextColor  (TFT_WHITE,TFT_BLACK);  
      tft.setTextSize   (2); 
      // set brightness
      ledcSetup         (pwmLedChannelTFT, pwmFreq, pwmResolution);//set brightness
      ledcAttachPin     (TFT_BL, pwmLedChannelTFT);
      ledcWrite         (pwmLedChannelTFT, backlight[1]);// 0-5 higher=brighter
#endif

  p1on();
  p2on();
  s1on();
  s2on();
  displaysetup();
  delay             (500);
  Serial.println    (__FILE__);//name of this doc 
  pinMode           (lbutton, INPUT);
  pinMode           (rbutton, INPUT);
  pinMode           (relayPin,OUTPUT);
  digitalWrite      (relayPin,LOW);
  setup_wifi();
  timeClient.begin(); //RTC Initialize a NTPClient to get time
  timeClient.setTimeOffset(3600);  
  whatsthetime();     //Create formatted day and timestamp
}
 
void loop() {
  unsigned long currentMillis = millis();  
  checkbuttons();  
  if(currentMillis - previousMillis  > interval) { //ping servers every 15 secs
    CheckSensorServers(0);
    if (sensor1serverup&&oping1==1){getSensorData(1,serverNameSoil) ;Serial.println("vochtigheid "+SoilValue[1]+"%");}// do it once when up
    if (sensor2serverup&&oping2==1){getSensorData(2,serverName2Soil);Serial.println("vochtigheid "+SoilValue[2]+"%");}
    humidity1=SoilValue[1].toInt();
    humidity2=SoilValue[2].toInt();
    if ((humidity1<=threshold) || (humidity2<=threshold)){water=true;}else{water=false;}
    if (debug){Serial.print("humidity1:");Serial.print(humidity1);Serial.print(" threshold:");Serial.print(threshold);Serial.print(" humidity2:");Serial.println(humidity2);}
    previousMillis = currentMillis; 
  }
  
  if (water!=oldwater){                             // set relay when changed
    if (water){digitalWrite(relayPin,HIGH);} else {digitalWrite(relayPin,LOW);}
    oldwater=water;
  }
     
  if(currentMillis - previousMillis2 > interval2) { // change screen every 5 secs
    previousMillis2 = currentMillis;
    displayscreen(screen);
    if (screen<numscreens){screen++;} else {screen=0;}
  }

  whatsthetime();
  if (TMinutes!=LastTMinutes){                      // check if a minute has gone
    OMinutes++;
    if (debug) {displaystatus();}   
    LastTMinutes=TMinutes;
    timesinces1++;
    timesinces2++;}
}

void displaystatus(){      
    Serial.print     ("running:");
    Serial.print     (OMinutes);
    Serial.print     ("min. S1times:");
    Serial.print     (s1online);
    Serial.print     (" last:");
    Serial.print     (timesinces1);
    Serial.print     ("min. avpings:");
    Serial.print     (avpings1,2);
    Serial.print     (" S2times:");
    Serial.print     (s2online);
    Serial.print     (" last:");
    Serial.print     (timesinces2);
    Serial.print     ("min. avpings:");
    Serial.println   (avpings2,2);
}

void setup_wifi() {
  WiFi.begin        (ssid, password);
  Serial.print      ("Connecting");
  while(WiFi.status() != WL_CONNECTED) {delay(250);Serial.print(".");}
  Serial.println    ("");
  Serial.print      ("Connected to ");
  Serial.print      (ssid);
  Serial.print      (" with IP Address: ");
  Serial.println    (WiFi.localIP());
}

void p1on(){          //ping 1 display
#if defined(mono)     //0.9 monochrome    
    u8g2.setFont      (u8g2_font_open_iconic_all_1x_t);//8 pix high
    u8g2.setCursor    (36,8);
    u8g2.print        (char(93));//bell
    u8g2.sendBuffer   ();
#else
    tft.setTextSize    (2); 
    tft.setTextFont    (1); 
    tft.setCursor      (14*12,0);
    tft.println        ("P1");
#endif
}
void p2on(){          //ping 2 display
#if defined(mono)     //0.9 monochrome    
    u8g2.setFont      (u8g2_font_open_iconic_all_1x_t); //8 pix high
    u8g2.setCursor    (36+16,8);
    u8g2.print        (char(93));                       //bell
    u8g2.sendBuffer   ();   
#else
    tft.setTextSize    (2); 
    tft.setTextFont    (1); 
    tft.setCursor      (17*12,0);
    tft.println        ("P2");
#endif
}
void s1on(){          //display s1 online
#if defined(mono)     //0.9 monochrome    
    u8g2.setFont      (u8g2_font_open_iconic_all_1x_t); //8 pix high
    u8g2.setCursor    (36+32,8);
    u8g2.print        (char(247));                      //wifi OK
    u8g2.sendBuffer   ();
#else
    tft.setTextSize    (2); 
    tft.setTextFont    (1); 
    tft.setCursor      (16*12,0);
    tft.setTextColor   (TFT_GREEN,TFT_BLACK);
    tft.println        ("*");
    tft.setTextColor   (TFT_WHITE,TFT_BLACK);

#endif
}
void s2on(){          //display s1 online
#if defined(mono)     //0.9 monochrome    
    u8g2.setFont      (u8g2_font_open_iconic_all_1x_t); //8 pix high
    u8g2.setCursor    (36+48,8);
    u8g2.print        (char(247));                      //wifi OK
    u8g2.sendBuffer   ();   
#else
    tft.setTextSize    (2); 
    tft.setTextFont    (1); 
    tft.setCursor      (19*12,0);
    tft.setTextColor   (TFT_GREEN,TFT_BLACK);
    tft.println        ("*");
    tft.setTextColor   (TFT_WHITE,TFT_BLACK);
#endif
}

void CheckSensorServers(int server){//0=both 1=1 2-=2 verbose option?
  if (server==0||server==1){
    p1on();
    if (debug){Serial.print("Pinging Server1:");}
    ping1++;
    bool Sensor1_UP = Ping.ping(SServer1,3);
    if (!Sensor1_UP) {      
      if (sensor1serverup){ // was up now down
         Serial.print     ("S1 this time pings up:");
         Serial.println   (oping1);
         totopings1     = totopings1+oping1;
         avpings1       = totopings1/s1online;
         avtime1        = OMinutes  /s1online;
         Serial.print     ("S1 average   pings up:");
         Serial.println   (avpings1);
         Serial.print     ("S1 average int. min. :");
         Serial.println   (avtime1);
         oping1=0;
         if (!debug)      {displaystatus();}   
       }
       if (debug)         {Serial.println     ("Sensor1 down  ");}
       sensor1serverup  = false;
      }
    else {
      s1on();
      oping1++; 
      Serial.print        ("Sensor1 online#");
      Serial.print        (oping1);
      Serial.print        (" humidity1:");
      Serial.println      (humidity1);
      if (!sensor1serverup){s1online++;}
      sensor1serverup   = true;
      timesinces1       = 0;
    } 
    if (debug) {Serial.println("Ping finished");}
  }
  if (server==0||server==2){
    p2on();
    ping2++;
    if (debug){Serial.print("Pinging Server2:");}
    bool Sensor2_UP = Ping.ping(SServer2,3);
    if (!Sensor2_UP) {
      if (sensor2serverup){ // was up now down
         Serial.print     ("S2 this time pings up:");
         Serial.println   (oping2);
         totopings2   =   totopings2+oping2;
         avpings2     =   totopings2/s2online;
         avtime2      =   OMinutes  /s2online;
         Serial.print     ("S2 average   pings up:");
         Serial.println   (avpings2);
         Serial.print     ("S2 average int. min. :");
         Serial.println   (avtime2);
         oping2=0;
       }
       if (debug){  Serial.println     ("Sensor2 down  ");}
       sensor2serverup=false;}
    else             {
        s2on();
        oping2++;         //online ping
        Serial.print      ("Sensor2 online");
        Serial.print      (oping2);
        Serial.print      (" humidity2:");
        Serial.println    (humidity2);
        if (!sensor2serverup){s2online++;}//was false && now true count as timesonline
        sensor2serverup = true;
        timesinces2     = 0;
      }    
    if (debug){Serial.println("Ping finished");}
  }
}
void whatsthetime(){
  while(!timeClient.update()) {timeClient.forceUpdate();}
  // 2018-05-28T16:00:13Z
  formattedDate   = timeClient.getFormattedDate();
  int splitT      = formattedDate.indexOf("T");
  dayStamp        = formattedDate.substring(0, splitT);
  TimeStamp       = formattedDate.substring(splitT+1, formattedDate.length()-1);
  STHours         = TimeStamp.substring(0,2);//"12:38"
  STMinutes       = TimeStamp.substring(3,5);
  int Thours      = STHours.toInt();
  int Tmins       = STMinutes.toInt();
  TMinutes        = (Thours*60)+Tmins;
}

void displaystring  (String value){
#if defined(mono)     //0.9 monochrome    
  u8g2.clearBuffer  ();    
  u8g2.setCursor    (0,32);
  u8g2.print        (value);
  u8g2.sendBuffer   ();    
#else
  tft.println       (value);
#endif
}

void displayscreen(int scr){
 switch (screen)    {
    case 0:{ 
      screen34(3,timesinces1,humidity1);
    break;}    
    case 1:{ 
      screen34(4,timesinces2,humidity2);  
    break;}
    case 2:{ 
      displaysetup();
      #if defined(mono)     //0.9 monochrome   
        u8g2.setFont      (u8g2_font_open_iconic_all_4x_t);//32 pix high
        u8g2.setCursor    (96,32);
        if (humidity1<=threshold||humidity2<=threshold){u8g2.print(char(152));}//drop    
        u8g2.sendBuffer();  
      #else
         tft.setTextSize    (5); 
         if (water){tft.setTextColor(TFT_RED,TFT_BLACK)  ;tft.print(" DRY");}
         else      {tft.setTextColor(TFT_GREEN,TFT_BLACK);tft.print(" WET");}
         tft.setTextColor   (TFT_WHITE,TFT_BLACK);
         tft.setTextSize    (2); 
      #endif 
    break;}   
    case 3:{ 
      screen12(1,timesinces1,humidity1,avtime1,avpings1);
    break;}    
    case 4:{ 
      screen12(2,timesinces2,humidity2,avtime2,avpings2);  
    break;}
 }
}  

void getSensorData      (int sensor,String URL){
     String soilhum     = httpGETRequest        (URL);
     int    pos         = soilhum.indexOf       (".");
     String tsoil       = soilhum.substring     (0, pos);
     if (tsoil!="--")   {SoilValue[sensor]=tsoil;} else {Serial.println("Invalid server response.");} //server went down 
     if (debug)         {Serial.print("S");Serial.print(sensor);Serial.print(":tsoil string ");Serial.println(tsoil);}     
}// end getsensordata

String httpGETRequest(String serverName) {
  WiFiClient client;
  HTTPClient http;
  http.begin           (client, serverName);
  httpResponseCode     = http.GET();
  String payload       = "--";  
  if (httpResponseCode>0) {
    if (debug)        {Serial.print("HTTP Response code: ");Serial.println(httpResponseCode);}  
    payload           = http.getString();
  } else {
    if (debug)        {Serial.print("Error code: ");Serial.println(httpResponseCode);}  
  }
  // Free resources
  http.end();
  return payload;
}

void   checkbuttons(){
#if defined(mono)                       // 0.9 monochrome   
  u8g2.setFont(u8g2_font_courB24_tf);   // bigger font
  bool pressed = true;
#else
  bool pressed = false;
#endif
if ((digitalRead (lbutton)==pressed)&&(digitalRead (rbutton)==pressed)){// left&right

      Serial.print  ("left:");
      Serial.print  (digitalRead (lbutton));
      Serial.print  (" right:");
      Serial.println(digitalRead (rbutton));
     
     if (!both){ // MODE WAS NORMAL 3 SCREENS
       displaystring (">EXTRA");
       both=true;
       numscreens = numscreens + 2;
       delay(1000);
     } else {
       displaystring (">NORM<");
       both=false;
       numscreens = numscreens = 2;
       delay(1000);
     }
   } else {
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
    }
  }
}

/* open_iconic_all font special chars
 *  
 *  ARROW DO  73
 *  ARROW UP  76
 *  BELL      93
 *  OK       120
 *  NOK      121
 *  CLOCK    123
 *  SETUP    129
 *  DROP     152
 *  HEART    183
 *  INFINITE 187
 *  BROKEN   197
 *  CHAIN    198
 *  BLOCK    217
 *  WIFI OK  247
 */

void displaysetup(){     // display setup threshold
#if defined(mono) //0.9 mono
    u8g2.clearBuffer  ();
    u8g2.setFont      (u8g2_font_open_iconic_all_4x_t);//32 pix high
    u8g2.setCursor    (0,32);
    u8g2.print        (char(129));//setup 
    u8g2.setCursor    (32+4,32);
    u8g2.setFont      (u8g2_font_courB24_tf);       
    u8g2.print        (threshold);
    u8g2.print        ("%");
    u8g2.sendBuffer   ();
#else
    tft.fillScreen     (TFT_BLACK);   
    tft.setTextSize    (5); 
    tft.setTextFont    (1); 
    tft.setCursor      (0,20);
    tft.println        ("SETUP");
    tft.setTextSize    (5); 
    tft.setCursor      (0,80);
    tft.print          (threshold);
    tft.print          ("%");   
    tft.setTextSize    (2); 
#endif    
}

void screen12(int s, int ts, int hm, float at,float ap){          // detailed screen when 2 buttons pressed
#if defined(mono) //0.9 mono
  u8g2.clearBuffer  (); 
  u8g2.setCursor    (0,32);
  u8g2.setFont      (u8g2_font_open_iconic_all_1x_t);             //8 pix high
  if (ap>0){for (int b=0;b<ap;b++){u8g2.print(char(93));}}        // bell or nothing
  u8g2.setCursor      (48,32); // 00000 111111111
  int hrs = at/30;
  if (hrs>0){for (int c=0;c<hrs;c++){u8g2.print(char(123));}}     // clock or nothing
    u8g2.setCursor    (0,21); 
  if (s==1) {
    u8g2.print(char(76));//arrow up  = back garden
  } else {
    u8g2.print(char(73));//arrow down= front garden
  }    
  u8g2.setFont      (u8g2_font_courB08_tf);
  u8g2.print        (":");
  u8g2.setFont      (u8g2_font_courB14_tf);
  u8g2.print        (SoilValue[s]);
  u8g2.print        ("%");
  u8g2.setFont      (u8g2_font_open_iconic_all_2x_t);
  if ((s==1&&sensor1serverup)||(s==2&&sensor2serverup)){u8g2.print(char(247));}//WIFI SYMBOL 
  else { 
    if (ts<75){
      u8g2.print    (char(183));                                  //HEART 
    } else { 
      u8g2.print    (char(197));                                  //CHAIN BROKEN
    }
  }
  if (ts>150){u8g2.print(char(187));}                             // twice missed infinite
  if (hm<=threshold){u8g2.print(char(152));}                      // drop    
  u8g2.setFont      (u8g2_font_courB14_tf);                       // 11 pix high
  u8g2.print        (ts);
  u8g2.sendBuffer   (); 
#else
    tft.fillScreen     (TFT_BLACK);      
    tft.setTextSize    (2); 
    tft.setTextFont    (1);         
    tft.setCursor      (0,0);
    tft.print          ("av:");
    tft.print          (ap,0);
    tft.print          (" ");
    tft.print          (at,0);
    tft.print          (" min.");
    tft.setTextSize    (5); 
    tft.setCursor      (0,20);
    tft.print          ("S");
    tft.print          (s);
    tft.print          (" ");
    if (hm<=threshold){tft.setTextColor(TFT_RED,TFT_BLACK);}else{tft.setTextColor(TFT_GREEN,TFT_BLACK);}
    tft.print          (hm);
    tft.print          ("%");
    tft.setCursor      (0,80);
    if ((s==1&&sensor1serverup)||(s==2&&sensor2serverup)){tft.setTextColor(TFT_GREEN,TFT_BLACK);tft.print("UP ");} 
    else {
      if (ts<75){tft.setTextColor(TFT_GREEN,TFT_BLACK);tft.print("OK");
      } else    {tft.setTextColor(TFT_RED,TFT_BLACK);  tft.print("LOST");}}
    tft.setTextSize    (2);  
    tft.setTextColor(TFT_WHITE,TFT_BLACK);
#endif   
if (s=1&&sensor1serverup){s1on();}
if (s=2&&sensor2serverup){s2on();}
}

void screen34(int s,int ts, int hm){
#if defined(mono) //0.9 mono
  u8g2.clearBuffer  ();//example back 11% NOK WIFI LOST    
  u8g2.setFont      (u8g2_font_courB24_tf);      
  u8g2.setCursor    (32+3,32);
  u8g2.print        (hm);
  u8g2.print        ("%");
  u8g2.setFont      (u8g2_font_open_iconic_all_4x_t);//32 pix high
  u8g2.setCursor    (0,32);
  if (s==3) {
    u8g2.print(char(76));                                     //arrow up  = back garden
  } else {
    u8g2.print(char(73));                                     //arrow down= front garden
  }
  u8g2.setCursor    (96,32);  
  if ((s==3&&sensor1serverup)||(s==4&&sensor2serverup)){u8g2.print(char(247));}//WIFI SYMBOL
  else {
    if (ts<75){
      u8g2.print        (char(183));//HEART 
    } else { 
      u8g2.print        (char(197));//CHAIN BROKEN
    }
  }
  u8g2.sendBuffer   ();    
#else
    tft.fillScreen     (TFT_BLACK);   
    tft.setTextFont    (1);     
    tft.setTextSize    (5); 
    tft.setCursor      (0,20);
    tft.print          ("S");
    tft.print          (s-2);
    tft.print          (" ");
    if (hm<=threshold){tft.setTextColor(TFT_RED,TFT_BLACK);}else{tft.setTextColor(TFT_GREEN,TFT_BLACK);}    
    tft.print          (hm);
    tft.print          ("%");
    if (water){tft.setTextColor(TFT_RED,TFT_BLACK);tft.print("W");}
    tft.setTextColor(TFT_WHITE,TFT_BLACK);
    tft.setCursor      (0,80);
    if ((s==1&&sensor1serverup)||(s==2&&sensor2serverup)){tft.setTextColor(TFT_GREEN,TFT_BLACK);tft.print("UP ");} 
    if (ts<75){tft.setTextColor(TFT_GREEN,TFT_BLACK);tft.print("OK ");
    } else    {tft.setTextColor(TFT_RED,TFT_BLACK);  tft.print("LOST ");}
    tft.print(ts);
    tft.setTextColor(TFT_WHITE,TFT_BLACK);   
    tft.setTextSize    (2);       
#endif   
if (sensor1serverup){s1on();}
if (sensor2serverup){s2on();}
}
