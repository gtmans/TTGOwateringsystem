/*
  The goal of this project is to create a sensoring system that controls my watering system (a rainbird raincomputer)
  Whenever one of the soilsensors signals the ground is to dry it triggers a relay that gives input to the raincomputer  

  First I needed to build a testing system. I started with these 3 modules of prox 12 euro each
  - LilyGO TTGO T-Display V1.1 ESP32 - met 1.14 inch TFT Display
  - LilyGO TTGO T-Higrow ESP32 - BME280 Sensor
  - LilyGO TTGO T-Higrow ESP32 - DHT11 Sensor (its easier to buy 2 of the same Higrows but I got stuck with these)

  I created 2 HTTP servers and one HTTP client inspired by  
  https://RandomNerdTutorials.com/esp32-client-server-wi-fi/
  https://github.com/Xinyuan-LilyGO/TTGO-LoRa-Series 
  https://gist.github.com/jenschr/0fc981415233e0751f22972811b4957f

  Program myESP32_server_Higrow_DHT11  Sensorserver1 IP 192.168.2.10 sleeps 70 awake 2 mins
  Program myESP32_server_Higrow_BME280 Sensorserver2 IP 192.168.2.11 never sleeps RED
  
  The sensors will be placed outdoors and will be battery based so to make them power efficient 
  I let hem sleep as long as possible (70 minutes was the best 4 me). So it wakes up every 70 minutes
  and is a server for 2 minutes then goes 2 sleep A 3D printed case can be foud on Thingiverse or Github
  
  Program myESP32_client_T-Display (this program)
  


  v9.33 serverName2Pres
  v9.34 HTTPClient fro char to string
*/

#include <EEPROM.h>
#include <SPI.h>
#include <TFT_eSPI.h>           // Hardware-specific library
TFT_eSPI tft = TFT_eSPI();      // Invoke custom library
#include <WiFi.h>
#include <ESP32Ping.h>
#include <HTTPClient.h>
#include "gewoon_secrets.h"
//RTC
#include <ArduinoJson.h>        //https://github.com/bblanchon/ArduinoJson.git
#include <NTPClient.h>          //https://github.com/taranais/NTPClient
#include <WiFiUdp.h>
String Ftemp;                   //formatted temp sensor1
String Fhum;                    //formatted hum  sensor1
String Fbar;                    //formatted bar  sensor1
String Fsoil;                   //formatted soilhumidity sensor1
String Ftemp2;                  //formatted temp sensor2
String Fhum2;                   //formatted hum  sensor2
String Fsoil2;                  //formatted soilhumidity sensor2
String resultaat;
bool   debug    = false;        // use for more info on serial monitor
bool   go2sleep = false;        // if true reads values from eeprom and writes them before going to sleep
bool   bonus    = true;
//HTTP
const String endpoint = "http://api.openweathermap.org/data/2.5/weather?q="+town+","+Country+"&units=metric&APPID=";
String payload  = "";           //whole json 
String tmp      = "";           //temperatur
String hum      = "";           //humidity
String tmpmi    = "";           //temp max
String tmpma    = "";           //temp min
String tmps     = "";           //windspeed
String tmpr     = "";           //wind direction
StaticJsonDocument<1000> doc;
WiFiUDP ntpUDP;                 // Define NTP Client to get time
NTPClient timeClient(ntpUDP);   // Variables to save date and time

int    timesince  =0;
int    timesinces1=0;
int    timesinces2=0;
int    TMinutes;
int    TMinutesIW;
int    TMinutesS1;
int    TMinutesS2;
int    LastTMinutes=0;
String STHours;
String STMinutes;
String formattedDate;
String dayStamp;
String TimeStamp;
String LastTimeStamp;
String Ftempi;                   //formatted internet temp
String Fhumi;                    //formatted internet hum
String Fbari;                    //formatted bar
String Fmin;
String Fmax;
float  windspeed;
int    beauf;
int    winddeg;
String winddirection;
String windtxt[]={"stil","zeer zwak","zwak","vrij matig","matig","vrij krachtig","krachtig","hard","stormachtig","storm","zware storm","zeer zwarestorm","orkaan"};
String winddir[]={"N","N","NNO","NNO","NO","NO","ONO","ONO","O","O","O","O","OZO","OZO","ZO","ZO","ZZO","ZZO","Z","Z","Z","Z","ZZW","ZZW","ZW","ZW","WZW","WZW","W","W","W","W","WNW","WNW","NW","NW","NNW","NNW","N","N"};
//SENSORS                 
String IPServer1       = "192.168.2.10";                  // IP address of 1st sensorserver
String IPServer2       = "192.168.2.11";                  // IP address of 2nd sensorserver
String Sensor1Server   = "http://"+IPServer1;
String Sensor2Server   = "http://"+IPServer2;
String serverNameTemp  = Sensor1Server + "/temperature";
String serverNameHumi  = Sensor1Server + "/humidity";
String serverNameSoil  = Sensor1Server + "/soilhum";
String serverName2Temp = Sensor2Server + "/temperature";
String serverName2Humi = Sensor2Server + "/humidity";
String serverName2Pres = Sensor2Server + "/pressure";
String serverName2Soil = Sensor2Server + "/soilhum";
//TFT
#include                    "Orbitron_Medium_20.h"
const int pwmFreq           = 5000;
const int pwmResolution     = 8;
const int pwmLedChannelTFT  = 0;
int backlight[5]            = {10,30,60,120,220};
//SCREENVARS
int orientation;
int screen                  = 0;//which data screen to show
#define   portrait            0
#define   landscape           3
#define   leftbutton          0
#define   rightbutton         35
//COLORS
#define   BLACK               0x0000
#define   BLUE                0x001F
#define   LBLUE               0x01E9
#define   RED                 0xF800
#define   GREEN               0x07E0
#define   CYAN                0x07FF
#define   MAGENTA             0xF81F
#define   YELLOW              0xFFE0
#define   WHITE               0xFFFF
//SCREENLOCATIONS
int     bar1data[4]={50,220,14,LBLUE};        //barpos x,y,h,blue
byte    bar1=1;
int     bar2data[4]={0 ,220,14,GREEN};        //bar2pos
byte    bar2=1;
int     item1data[2]={120,0};                 //pos x,y screen display
int     item2data[2]={90,220};                //time since
int     item3data[2]={50 ,220};               //HTTP BARS
int     item4data[2]={20 ,200};               //BIG HUMIDITY
int     weerkleur   =TFT_MAGENTA;
int     sensor1kleur=TFT_YELLOW;
int     sensor2kleur=TFT_GREEN;
#define posx 0                                // for easy adressing barxdata[posx,posy,hgt,col]
#define posy 1
#define hgt  2
#define col  3

String  temperature;
String  humidity;
String  pressure;
String  soilhum;
String  temperature2;
String  humidity2;
String  soilhum2;
const long    interval        = 300000; // check sensor1
const long    interval2       = 300000; // check sensor2 
const long    interval3       = 3*300000; // check web weather every 15 mins
const long    offset          = 30000;  // do not do all the checks at the same time
const long    swapinterval    = 10000;  // swap screen in seconds
const long    sleeptime       = 300000; // go2sleep
const long    serverinterval  = 300000; // check servers online every 5 mins
unsigned long previousMillis  = 0;
unsigned long previousMillis2 = 0;
unsigned long previousMillis3 = 0;
unsigned long previousMillis4 = 0;
unsigned long previousMillis5 = 0;       //swaptime
unsigned long previousMillis6 = 0;       //serverinterval
bool          weatherchange   = true;
bool          timechange      = true;
bool          sensor1change   = true;
bool          sensor2change   = false;
bool          sensor1serverup;
int           sensor1uptime;
int           sensor1tries    = 0;
bool          sensor2serverup;
int           sensor2uptime;
int           sensor2tries    = 0;
int           httpResponseCode= 200;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println    (__FILE__);//name of this doc 
  String longname = (__FILE__);
  int    pos      = longname.length();
  String shortname= longname.substring(pos-25);
  Serial.println    (shortname);
  pinMode(leftbutton, INPUT);
  pinMode(rightbutton,INPUT);
  
  tft.init();
  tft.setRotation   (landscape);
  tft.fillScreen    (TFT_BLACK);
  tft.setTextColor  (TFT_WHITE,TFT_BLACK);  
  tft.setTextSize   (2); 
  ledcSetup         (pwmLedChannelTFT, pwmFreq, pwmResolution);//set brightness
  ledcAttachPin     (TFT_BL, pwmLedChannelTFT);
  ledcWrite         (pwmLedChannelTFT, backlight[bar1]);

bool  DHCPIP=true;                      // get client IP from DHCP server
  if (DHCPIP==false){
    IPAddress local_IP    (192, 168,   2,   9); // Set your Static IP address
    IPAddress gateway     (192, 168,   2, 254); // Set your Gateway IP address
    IPAddress subnet      (255, 255, 255,   0);
    IPAddress primaryDNS  (8, 8, 8, 8);         //optional
    IPAddress secondaryDNS(8, 8, 4, 4);         //optional
    if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)){// Configures static IP address
      Serial.println("STA Failed to configure");}      
  }
  WiFi.begin        (ssid, password);
  tft.print         ("Connecting");
  Serial.print      ("Connecting");
  while(WiFi.status() != WL_CONNECTED) { 
    delay(500);
    Serial.print    (".");
    tft.print       (".");
    }
  Serial.println    ("");
  Serial.print      ("Connected to ");
  tft.print         ("Connected to ");
  Serial.print      (ssid);
  tft.print         (ssid);
  Serial.print      (" with IP Address: ");
  tft.print         (" with IP Address: ");
  Serial.println    (WiFi.localIP());
  tft.println       (WiFi.localIP());

  if (go2sleep==true){readvalues();}
   
  CheckSensorServers(0);
  delay             (1000);  
  tft.setTextSize   (2);
  tft.setRotation   (portrait);
  orientation =     portrait;

  timeClient.begin(); //RTC Initialize a NTPClient to get time
  timeClient.setTimeOffset(3600);  
  whatsthetime();     //Create formatted day and timestamp
  
  if (debug==true&&go2sleep==true){
    Serial.println   ("voor:");
    Serial.print    ("Ftemp:");
    Serial.print     (Ftemp);   //formatted temp sensor1
    Serial.print    ("Fhum");
    Serial.print     (Fhum);
    Serial.print    ("Fsoil:");
    Serial.print     (Fsoil);
    Serial.print    ("Fbar:");
    Serial.println   (Fbar);  
    Serial.print    ("Ftemp2:");
    Serial.print     (Ftemp2);   //formatted temp sensor1
    Serial.print    ("Fhum2");
    Serial.print     (Fhum2);
    Serial.print    ("Fsoil2:");
    Serial.println   (Fsoil2);
  } 
  tft.fillScreen     (TFT_BLACK);   
  getWeatherData();
  getSensorData (); 
  getSensor2Data();
  LastTMinutes=TMinutes;
//  if (TMinutesS1>TMinutes){TMinutesS1=990;}// 1440 value is from yesterday
//  if (TMinutesS2>TMinutes){TMinutesS2=990;}// make it a day old       
    
  if (debug==true&&go2sleep==true){
    Serial.println   ("na:");
    Serial.print    ("Ftemp:");
    Serial.print     (Ftemp);   //formatted temp sensor1
    Serial.print    ("Fhum");
    Serial.print     (Fhum);
    Serial.print    ("Fsoil:");
    Serial.print     (Fsoil);
    Serial.print    ("Fbar:");
    Serial.println   (Fbar);  
    Serial.print    ("Ftemp2:");
    Serial.print     (Ftemp2);   //formatted temp sensor1
    Serial.print    ("Fhum2");
    Serial.print     (Fhum2);
    Serial.print    ("Fsoil2:");
    Serial.println   (Fsoil2);
  }
} // setup

void loop() {
  unsigned long currentMillis = millis();  
  keypressed();
  whatsthetime();
  if (TMinutes!=LastTMinutes){        // 1 min gone
     Serial.print              (">>>>> sensor1server:");
     if (sensor1serverup==false){     // check sensor1 if down
        Serial.print           ("DOWN->");
        CheckSensorServers(1);
        if (sensor1serverup==true){   // get sensor1data
           Serial.println      ("UP");
           getSensorData (); 
        } /*else {Serial.print   ("S1 Last seen ");
                Serial.print   (TMinutes-TMinutesS1);     
                Serial.println (" minutes ago.");
        }*/
     } else    {Serial.println     ("UP");}
     
     Serial.print              (">>>>> sensor2server:");
     if (sensor2serverup==false){     // check sensor2 if down
        Serial.print           ("DOWN->");
        CheckSensorServers(2);
        if (sensor2serverup==true){   // get sensor1data
           Serial.println      ("UP");
           getSensor2Data (); 
        } /*else {Serial.print   ("S2 Last seen ");
                Serial.print   (TMinutes-TMinutesS2);     
                Serial.println (" minutes ago.");
        }*/
     } else    {Serial.println     ("UP");}
    
    LastTMinutes=TMinutes; 
    if (bonus==true){
      Serial.print  ("TMinutes:");
      Serial.print  (TMinutes);
      Serial.print  (" TMinutesS1:");
      Serial.print  (TMinutesS1);
      Serial.print  (" TMinutesS2:");
      Serial.print  (TMinutesS2);
      Serial.print  (" TMinutesIW:");
      Serial.println(TMinutesIW);
      Serial.print  (TimeStamp);
      Serial.print  (" Time since: ");
      Serial.print  (TMinutes-TMinutesS1);
      Serial.print  (" / ");
      Serial.print  (TMinutes-TMinutesS2);
      Serial.print  (" / ");
      Serial.print  (TMinutes-TMinutesIW);
      Serial.println(" minutes");  
    }
  }
         
  if(currentMillis - previousMillis > interval) {
    if (debug==true||bonus==true){
      Serial.print  (TimeStamp);
      Serial.println("****************** trigger sensor 1 ******************");
      if (debug==true){Serial.println(currentMillis - previousMillis);}
    }
    getSensorData();
    sensor1change  = true;
    previousMillis = currentMillis;
  }
 
  if(currentMillis + offset - previousMillis2 > interval2) {
     if (debug==true||bonus==true){
      Serial.print  (TimeStamp);
      Serial.println("****************** trigger sensor 2 ******************");
     if (debug==true){Serial.print (currentMillis - previousMillis2);}
    }
    getSensor2Data();
    sensor2change  = true;
    previousMillis2 = currentMillis;
  }
  
  if(currentMillis + (offset*2)  - previousMillis3 > interval3) {
    if (debug==true||bonus==true){
      Serial.print  (TimeStamp);
      Serial.println("****************** trigger weather ******************");
      if (debug==true){Serial.print (currentMillis - previousMillis3);}
    }
    getWeatherData();
    weatherchange   = true;
    previousMillis3 = currentMillis;
  }

  if (go2sleep==true){
    if(currentMillis - previousMillis4 > sleeptime) {
      if (debug==true){
        Serial.print  (currentMillis);
        Serial.print  ("<current previous4>");
        Serial.println(previousMillis4);
      }
      Sleep();
    }
  }

  if(currentMillis - previousMillis5 > swapinterval) {
    if (debug==true){
      Serial.print  (currentMillis);
      Serial.print  ("<current previous5>");
      Serial.println(previousMillis5);
    }
    previousMillis5 = currentMillis;
    nextscreen();
  }

  if(currentMillis - previousMillis6 > serverinterval) {
    if (debug==true){
      Serial.print  (currentMillis);
      Serial.print  ("<current previous6>");
      Serial.println(previousMillis6);
    }
    previousMillis6 = currentMillis;
    if (sensor1serverup==false){CheckSensorServers(1);}
    if (sensor2serverup==false){CheckSensorServers(2);}
  }

  if (TimeStamp!=LastTimeStamp){timechange=true;}
  tft.setTextSize              (2);    
  if (orientation==portrait){    
  tft.setTextFont              (1); 
    tft.setCursor              (item1data[posx]-24,item1data[posy]);
    if (sensor1serverup==true){
      tft.setTextColor         (TFT_GREEN,TFT_BLACK);
    } else { 
      tft.setTextColor         (TFT_RED,  TFT_BLACK);
    }
    tft.print                  ("*");
    if (sensor2serverup==true){
      tft.setTextColor         (TFT_GREEN,TFT_BLACK);
    } else { 
      tft.setTextColor         (TFT_RED,  TFT_BLACK);
    }
    tft.print                  ("*");
    tft.setCursor              (item1data[posx]-24,item1data[posy]+20);
    if (TMinutes-TMinutesS1<=73) {
      tft.setTextColor         (TFT_GREEN,TFT_BLACK);
    } else { 
      tft.setTextColor         (TFT_RED,  TFT_BLACK);
    }
    tft.print                  ("X"); 
    if (TMinutes-TMinutesS2<=73) {
      tft.setTextColor         (TFT_GREEN,TFT_BLACK);
    } else { 
      tft.setTextColor         (TFT_RED,  TFT_BLACK);
    }
    tft.print                  ("X"); 
  
  } // portrait
  tft.setTextSize              (1);
  tft.setTextColor             (TFT_WHITE,TFT_BLACK);

  switch (screen) {
    
    case 0:{                   //internettijd

      if (orientation==landscape){    
           tft.setCursor      (0,0); 
           tft.setTextSize    (4);    
           tft.setTextFont    (1);
           tft.println        ("Apeldoorn time");
           String current =   TimeStamp.substring(0,8);
           tft.print          (current);
       }  else {               
           tft.setTextSize    (2);    
           tft.setTextFont    (1); 
           tft.setCursor      (item1data[posx],item1data[posy]);
           tft.print          (screen);
           tft.setCursor      (item2data[posx]-12,item2data[posy]);
           tft.print          ("Time");
           if                 (timechange==true){ 
            tft.setTextSize   (1);
            tft.setFreeFont   (&Orbitron_Medium_20);
            tft.setTextColor  (TFT_CYAN,TFT_BLACK);
            tft.setCursor     (0,16);
            tft.println       ("Internet");   
            tft.setTextColor  (TFT_GREEN,TFT_BLACK);
            tft.println       ("tijd in");
            tft.println       ("Apeldoorn");         
            tft.setTextSize   (1);
            tft.setFreeFont   (&Orbitron_Light_24);
            tft.setTextColor  (TFT_WHITE,TFT_BLACK);
            String current =  TimeStamp.substring(0,8);
            tft.fillRect      (0,100,128,30,TFT_BLACK);
            tft.setCursor     (0,125);
            tft.println       (current);
            tft.println       (dayStamp);
            tft.setTextSize   (2);
            tft.setTextFont   (2);
            timechange      = false;     
           }//end of timechange
       }// end of portrait
    break;}
    
    case 1:{                //internetweer 
      if (orientation==landscape){    
           tft.setCursor      (0,0); 
           tft.setTextSize    (4);    
           tft.setTextFont    (1);
           tft.print          (Ftempi);
           tft.println        (" C");
           tft.print          (Fmin);
           tft.println        (" C min.");
           tft.print          ("wind ");
           tft.print          (winddirection);
           tft.println        (beauf);
           tft.println        (windtxt[beauf]);          
       } else {  // portrait         
           tft.setTextSize    (2);    
           tft.setTextFont    (1); 
           tft.setCursor      (item1data[posx],item1data[posy]);
           tft.print          (screen);
           tft.setCursor      (item2data[posx],item2data[posy]);
           tft.print          (TMinutes-TMinutesIW);        //time since last update
    
           if   (weatherchange==true){ 
              if (debug==true){Serial.println( "weatherchange");}
              tft.fillScreen    (TFT_BLACK);
              tft.setTextColor  (weerkleur,TFT_BLACK);
              DrawBars          ();  
              tft.setTextSize   (2);    
              tft.setTextFont   (2); 
              tft.setCursor     (80,70);
              tft.print         ("C"); 
              tft.setCursor     (80,95);
              tft.print         ("min");
              tft.setCursor     (80,120);
              tft.print         ("max");
              tft.setCursor     (80,145);
              tft.print         ("%");                  
              tft.setTextSize   (1);
              tft.setFreeFont   (&Orbitron_Medium_20);
              tft.setTextColor  (TFT_CYAN,TFT_BLACK);
              tft.setCursor     (0,16);
              tft.println       ("Internet");   
              tft.setTextColor  (TFT_GREEN,TFT_BLACK);
              tft.println       ("weer in");
              tft.println       ("Apeldoorn");
              tft.println       ();         
              tft.setTextSize   (1);
              tft.setFreeFont   (&Orbitron_Light_24);
              tft.setTextColor  (TFT_MAGENTA,TFT_BLACK);             
              tft.println       (Ftempi);
              tft.println       (Fmin);
              tft.println       (Fmax);
              tft.println       (Fhumi);        
              tft.print         (winddirection);
              tft.println       (beauf);
              tft.println       (windtxt[beauf]);
              tft.setTextFont   (2); 
              tft.setTextSize   (2);           
              weatherchange=false;    
          }//end of weatherchange
     }// end of portrait
    break;}
    
    case 2:{                //outsidesensor1
      if (orientation==landscape){    
         tft.setCursor      (0,0); 
         tft.setTextSize    (4);    
         tft.setTextFont    (1);
         tft.println        ("Sensor1");
         tft.setTextColor   (sensor1kleur,TFT_BLACK);
         tft.println        ();
         tft.setTextSize    (8);  
         tft.print          (Fsoil);
         tft.println        (" %");   
       } else {   
         tft.setTextSize    (2);    
         tft.setTextFont    (1); 
         tft.setCursor      (item1data[posx],item1data[posy]);
         tft.print          (screen);
         tft.setCursor      (item2data[posx],item2data[posy]);
         if (sensor1serverup==true){
             tft.print      (TMinutes-TMinutesS1);        //time since last update
         } else {
            tft.print       (TMinutes-TMinutesS1);        //time since last update 
         }
         
    if   (sensor1change==true){ 
          if (debug==true){Serial.println( "sensor1change");}
          tft.fillScreen    (TFT_BLACK);
          DrawBars();  
          tft.setTextColor  (sensor1kleur,TFT_BLACK);
          tft.setTextSize   (2);    
          tft.setTextFont   (2); 
          tft.setCursor     (80,70);
          tft.print         ("C"); 
          tft.setCursor     (80,95);
          tft.print         ("%");
          tft.setCursor     (80,120);
          tft.print         ("mb.");
          tft.setTextSize   (1);
          tft.setFreeFont   (&Orbitron_Medium_20);
          tft.setTextColor  (TFT_CYAN,TFT_BLACK);
          tft.setCursor     (0,16);
          tft.println       ("Binnen");   
          tft.setTextColor  (TFT_GREEN,TFT_BLACK);
          tft.println       ("sens1");
          tft.println       ("Schuur");
          tft.println       ();         
          tft.setTextSize   (1);
          tft.setFreeFont   (&Orbitron_Light_24);
          tft.setTextColor  (sensor1kleur,TFT_BLACK);
          tft.println       (Ftemp);
          tft.println       (Fhum);
          tft.print         (Fbar);
          tft.setCursor     (item4data[posx],item4data[posy]);
          tft.setFreeFont   (&Orbitron_Light_32);
          tft.print         (Fsoil);
          tft.println       (" %");
          tft.setTextFont   (2); 
          tft.setTextSize   (2);      
          sensor1change=false;  
        }//end of sensor1rchange
     }// end of portrait
    break;}
  
    case 3:{                //buitensensor2
       if (orientation==landscape){    
          tft.setCursor     (0,0); 
          tft.setTextSize   (4);    
          tft.setTextFont   (1);
          tft.println       ("Sensor2");
          tft.println       ();
          tft.setTextSize   (8);  
          tft.setTextColor  (sensor2kleur,TFT_BLACK);
          tft.print         (Fsoil2);
          tft.println       (" %");
       } else {   
          tft.setTextSize   (2);    
          tft.setTextFont   (1); 
          tft.setCursor     (item1data[posx],item1data[posy]);
          tft.print         (screen);
          tft.setCursor     (item2data[posx],item2data[posy]);     
          if (sensor2serverup==true){
            tft.print       (TMinutes-TMinutesS2);                    //time since last update
          } else { 
            tft.print       (TMinutes-TMinutesS2);
          }
                  
          if  (sensor2change==true){ 
           if (debug==true){Serial.println( "sensor2change");}
            tft.fillScreen  (TFT_BLACK);
            tft.setTextColor(sensor2kleur,TFT_BLACK);
            DrawBars        ();  
            tft.setTextSize (2);    
            tft.setTextFont (2); 
            tft.setCursor   (80,70);
            tft.print       ("C"); 
            tft.setCursor   (80,95);
            tft.print       ("%");     
            tft.setTextSize (1);
            tft.setFreeFont (&Orbitron_Medium_20);
            tft.setTextColor(TFT_CYAN,TFT_BLACK);
            tft.setCursor   (0,16);
            tft.println     ("Buiten");   
            tft.setTextColor(TFT_GREEN,TFT_BLACK);
            tft.println     ("sens2");
            tft.println     ("tuin");
            tft.println     ();
            tft.setTextColor(sensor2kleur,TFT_BLACK);
            tft.setTextSize (1);
            tft.setFreeFont (&Orbitron_Light_24);
            tft.println     (Ftemp2);
            tft.println     (Fhum2);
            tft.println     ();
            tft.setCursor   (item4data[posx],item4data[posy]);
            tft.setFreeFont (&Orbitron_Light_32);
            tft.print       (Fsoil2);
            tft.println     (" %");
            tft.setTextFont (2); 
            tft.setTextSize (2);                
            sensor2change=false;  
         }//end of sensor2rchange
     }// end of portrait
  break;}
  } //   end of switch
  LastTimeStamp=TimeStamp;
} //  end of loop

void getTMinutes(){
  STHours         = TimeStamp.substring(0,2);//"12:38"
  STMinutes       = TimeStamp.substring(3,5);
  int Thours      = STHours.toInt();
  int Tmins       = STMinutes.toInt();
  TMinutes        = (Thours*60)+Tmins;
  if (debug==true){
    Serial.print    ("TimeStamp:");
    Serial.print    (TimeStamp);
    Serial.print    ("TMinutes:");
    Serial.println  (TMinutes);
  }
}

void getSensorData(){
 if(WiFi.status()== WL_CONNECTED ){
    if (sensor1serverup==true){
        httpResponseCode=1; 
        bar1data[col] = TFT_YELLOW;//OK
        sensor1kleur  = TFT_YELLOW;
        DrawBars2Init   ();
        if (debug==true){Serial.println("sensor1kleur=TFT_YELLOW");}
        Serial.print    ("S1 Getting: ")       ;Serial.println  (serverNameTemp);
        if (httpResponseCode>0) { temperature   = httpGETRequest(serverNameTemp);}
        Serial.print    ("S1 Getting:")       ;Serial.println  (serverNameHumi);
        if (httpResponseCode>0) {  humidity     = httpGETRequest(serverNameHumi);}
        Serial.print    ("S1 Getting: ")       ;Serial.println  (serverNameSoil);
        if (httpResponseCode>0) {  soilhum       = httpGETRequest(serverNameSoil); }     
        if (httpResponseCode>0) {        
          int splitTemp = temperature.indexOf   (".");
          Ftemp         = temperature.substring (0, splitTemp+2);
          int humTemp   = humidity.indexOf      (".");
          Fhum          = humidity.substring    (0, humTemp+2);
          int barTemp   = pressure.indexOf      (".");
          Fbar          = pressure.substring    (0, barTemp);
          int soilTemp  = soilhum.indexOf       (".");
          Fsoil         = soilhum.substring     (0, soilTemp);
          if (debug == true){
           Serial.println  ("Temperature: " + temperature + " *C - Humidity: " + humidity + " % - Pressure: " + pressure + " hPa");
           Serial.println  ("Soilhum: " + soilhum);
           Serial.println  ("Temperature: " + Ftemp + " *C - Humidity: " + Fhum + " % - Pressure: " + Fbar + " hPa");
           Serial.println  ("Soilhum: " + soilhum);}
          timesinces1=0;
          getTMinutes();
          TMinutesS1=TMinutes;
          sensor1tries  = 0;
          sensor1serverup=true;
        } else { // responsecode <=0
          sensor1serverup=false;
       } //ERROR     
    }//sensorserver1 up
    if (sensor1serverup==true){ // check again!
       Serial.println  ("Sensor1server up!");        
     } else {
       sensor1kleur = TFT_RED  ;
       sensor1tries++;
       Serial.print    ("Sensor1server offline! ");
       sensor1uptime=0;
       sensor1tries++;
       Serial.print    ("pogingen:");
       Serial.println  (sensor1tries);
     }//sensorserver   
     sensor1change=true;
     if (debug==true){
       Serial.print    ("Last httpResponseCode:");
       Serial.println  (httpResponseCode);}
  } else {Serial.println("WiFi Disconnected");} // WIFI connected
}// end getsensordata

void getSensor2Data(){
  if(WiFi.status()== WL_CONNECTED ){ 
    if (sensor2serverup==true){
        httpResponseCode=1; 
        bar2data[col] = TFT_GREEN;//OK
        sensor2kleur  = TFT_GREEN  ;//OK
        DrawBars2Init   ();
        if (debug==true){Serial.println("sensor2kleur=TFT_GREEN2");}
        if (httpResponseCode>0) {Serial.print("S2 Getting:");Serial.println  (serverName2Temp); 
                                temperature2 =                 httpGETRequest(serverName2Temp);}
        if (httpResponseCode>0) {Serial.print("S2 Getting:");Serial.println  (serverName2Humi);
                                humidity2    =                 httpGETRequest(serverName2Humi);}
        if (httpResponseCode>0) {Serial.print ("S2 Getting:");Serial.println (serverName2Pres);
                                pressure     = httpGETRequest(serverName2Pres);}

        if (httpResponseCode>0) {Serial.print("S2 Getting:");Serial.println  (serverName2Soil);
                                soilhum2     =                 httpGETRequest(serverName2Soil);}         
        if (httpResponseCode>0) { 
          timesinces2   =0;
          int splitTemp2= temperature2.indexOf   (".");
          Ftemp2=         temperature2.substring (0, splitTemp2+2);
          int humTemp2=   humidity2.indexOf      (".");
          Fhum2=          humidity2.substring    (0, humTemp2+2);
          int soilTemp2=  soilhum2.indexOf       (".");
          Fsoil2=         soilhum2.substring     (0, soilTemp2);
       
          if (debug == true){
            Serial.println  ("Temperature2: " + temperature2 + " *C - Humidity2: " + humidity2);
            Serial.println  ("Soilhum2: " + soilhum2);
            Serial.println  ("Temperature2: " + Ftemp2 + " *C - Humidity2: " + Fhum2 + " % ");
            Serial.println  ("Soilhum2: " + soilhum2);}
         timesinces1=0;
         getTMinutes();
         TMinutesS2=TMinutes;// time since = TMinutes - TMinutesS2 
         sensor2tries  = 0;
         sensor2serverup=true;
       } else { // responsecode <=0
         sensor2serverup=false;
       } //ERROR     
    }//sensorserver2 up

        if (sensor2serverup==true){ // check again!
          sensor2kleur = TFT_GREEN  ;//OK
          Serial.println  ("Sensor2server up!");        
        } else {
          sensor2kleur = TFT_RED  ;
          sensor2tries++;
          Serial.print    ("Sensor2server offline! ");
          sensor2uptime=0;
          sensor2tries++;
          Serial.print    ("pogingen:");
          Serial.println  (sensor2tries);
        }//sensorserver     
      sensor2change=true;
      if (debug==true){
        Serial.print    ("Last httpResponseCode:");
        Serial.println  (httpResponseCode);}

   } else {Serial.println("WiFi Disconnected");} // WIFI connected
}// end getsensor2data

//String httpGETRequest(const char* serverName) {
  String httpGETRequest(String serverName) {
  WiFiClient client;
  HTTPClient http;
  http.begin             (client, serverName);
  // Send HTTP POST request
  httpResponseCode     = http.GET();
  String payload       = "--"; //?  
  if (httpResponseCode>0) {
    if (debug==true){
      Serial.print     ("HTTP Response code: ");
      Serial.println   (httpResponseCode);
    }
    payload            = http.getString();
    DrawBars2();
  }
  else {
    Serial.print      ("Error code: ");
    Serial.println    (httpResponseCode);
//    Serial.println    ("sensor2kleur=TFT_RED");
//    sensor2kleur=TFT_RED    ;//ERROR
    DrawBars2();
  }  
  // Free resources
  http.end();
  return payload;
}

void getWeatherData(){  
   if ((WiFi.status() == WL_CONNECTED)) { //Check the current connection status
    //    tft.setCursor(20,220);
    DrawBars2Init();
    bar2data[col]=TFT_MAGENTA;
    HTTPClient http; 
    http.begin(endpoint + key); //Specify the URL
    int httpCode = http.GET();  //Make the request 
    if (httpCode > 0) { //Check for the returning code 
         DrawBars2();
         payload = http.getString();
         if (debug==true){
         Serial.println(httpCode);
         Serial.println(payload);}        
     } else { Serial.println("Error on HTTP request");   
     bar2data[3]=RED;
     DrawBars2();
     bar2data[3]=MAGENTA;
    }
    http.end(); //Free the resources

//{"coord":{"lon":5.9694,"lat":52.21},"weather":[{"id":803,"main":"Clouds","description":"broken clouds","icon":"04d"}]
//,"base":"stations","main":{"temp":2.7,"feels_like":-1.04,"temp_min":1.57,"temp_max":4.36,"pressure":1001,"humidity":96}
//,"visibility":10000,"wind":{"speed":4.12,"deg":270},"clouds":{"all":75},"dt":1638438824,"sys":{"type":2,"id":2010138
//,"country":"NL","sunrise":1638429867,"sunset":1638458825},"timezone":3600,"id":2759706,"name":"Apeldoorn","cod":200}

    DrawBars2();
    char inp[1000];
    payload.toCharArray(inp,1000);
    deserializeJson(doc,inp);
    String tmp2  = doc["main"]["temp"];
    String hum2  = doc["main"]["humidity"];
    String town2 = doc["name"];
    String tmpmi2= doc["main"]["temp_min"];
    String tmpma2= doc["main"]["temp_max"];
    String tmps2 = doc["wind"]["speed"];
    String tmpr2 = doc["wind"]["deg"];
    DrawBars2();

    tmp   = tmp2;
    hum   = hum2;
    tmpmi = tmpmi2;
    tmpma = tmpma2;
    tmps  = tmps2;
    tmpr  = tmpr2;
    
    //convert to float
    windspeed       = tmps.toFloat();
    Beaufort          (windspeed);      // returs int beauf
    float windtemp  = tmpr.toFloat();
    int   windtemp2 = windtemp/10;
    winddirection   =  winddir[windtemp2];
    //text formatting    
    int splitTempi = tmp.indexOf    (".");
    Ftempi =         tmp.substring  (0, splitTempi+2);
    int humTempi =   hum.indexOf    (".");
    Fhumi =          hum.substring  (0, humTempi+3);
    int tmin     =   tmpmi.indexOf  (".");
    Fmin         =   tmpmi.substring(0,tmin+2);
    int tmax     =   tmpma.indexOf  (".");
    Fmax         =   tmpma.substring(0,tmax+2);
    //debug
    if (debug==true){
      Serial.println("Temperature"+String(tmp));
      Serial.println("Humidity"+hum);
      Serial.println(town);
      Serial.println("TempMin:   "+String(tmpmi));
      Serial.println("TempMax:   "+String(tmpma));
      Serial.println("Wind   :   "+String(tmps));
      Serial.println("winrich:   "+String(tmpr));
    }
    DrawBars2();     
    weatherchange=true;
    getTMinutes();
    TMinutesIW=TMinutes;          // timestamp = now     
 } else {Serial.println("WiFi Disconnected");}  
}

void keypressed(){
if(digitalRead          (rightbutton)==0){
   previousMillis4 = millis();
   int presr=0;
   while (digitalRead  (rightbutton)==0&&presr<75){delay(10);presr++;}
   Serial.print        (screen);
   Serial.print        (" presltime:");   
   Serial.println      (presr);  
   if (presr>=75){      //longpress
     if (orientation == portrait){orientation = landscape;} else {orientation = portrait;}
        tft.setRotation    (orientation);
        tft.fillScreen     (TFT_BLACK);                  
   } else {             //shortpress
    nextscreen(); 
    //previousMillis5 = millis();// reset swaptime
   }
}// end of rightbutton

if(digitalRead(leftbutton)==0){
  previousMillis4 = millis();
  int  presl=0;
  while (digitalRead(leftbutton)==0&&presl<75){delay(10);presl++;}
  Serial.print          ("presltime:");   
  Serial.println        (presl);   
  if (presl>=75){       //longpress
     tft.setCursor      (0,200);
     tft.print          ("getting ");
     switch (screen) {
       case 0:{            //internettijd
             tft.print     ("doing nothing");
             tft.fillScreen(TFT_BLACK); 
             break;}
       case 1:{            //internetweer 
             tft.print     ("W");
             previousMillis3 = millis();
             getWeatherData();
             break;}
       case 2:{            //sensor1
             tft.print     ("S1");
             previousMillis = millis();
             if (sensor1serverup==false){CheckSensorServers(1);}
             getSensorData();
       break;}
       case 3:{            //sensor2 
             tft.print     ("S2");
             previousMillis2 = millis();
             if (sensor2serverup==false){CheckSensorServers(2);}
             getSensor2Data();
       break;}
    }
    } else {                   //shortpress refresh all obsolete

   bar1++;if(bar1>=5){bar1=0;}
   DrawBars();  
   ledcWrite(pwmLedChannelTFT, backlight[bar1]);
    }
    while (digitalRead(leftbutton)==0){delay(10);}
  }
}

void nextscreen(){
     screen++; if        (screen>3){screen=0;}
     if (debug==true){
     Serial.print        ("changing to screen:");
     Serial.println      (screen);}
     switch              (screen) {
       case 0:{timechange   =true;tft.fillScreen(TFT_BLACK);break;} //time 
       case 1:{weatherchange=true;tft.fillScreen(TFT_BLACK);break;} //internetweer 
       case 2:{sensor1change=true;tft.fillScreen(TFT_BLACK);break;} //sensor1 
       case 3:{sensor2change=true;tft.fillScreen(TFT_BLACK);break;} //sensor2      
     }//
}

void DrawBars() {
  tft.fillRect(bar1data[posx],bar1data[posy],44,bar1data[hgt],TFT_BLACK); 
//bar1++;if(bar1>=5){bar1=0;}
  for(int i=0;i<bar1+1;i++){
  tft.fillRect(bar1data[posx]+(i*7),bar1data[posy],3,bar1data[hgt],bar1data[col]);}//blue 
}

void DrawBars2Init() {
  bar2=0;
  tft.fillRect(bar2data[posx],bar2data[posy],44,bar2data[hgt],TFT_BLACK); 
  }

void DrawBars2() {
  tft.fillRect(bar2data[posx]+(bar2*7),bar2data[posy],3,bar2data[hgt],bar2data[col]);//blue 
  bar2++;if(bar2>=5){bar2=0;}
  }

void Beaufort(float wspeed){
  if (debug==true){Serial.print  ("windspeed");Serial.println(wspeed);}
  if (wspeed<  0.3)               {beauf= 0;}
  if (wspeed>= 0.3 && wspeed< 1.6){beauf= 1;}
  if (wspeed>= 1.6 && wspeed< 3.4){beauf= 2;}
  if (wspeed>= 3.4 && wspeed< 5.5){beauf= 3;}
  if (wspeed>= 5.5 && wspeed< 8.0){beauf= 4;}
  if (wspeed>= 8.0 && wspeed<10.8){beauf= 5;}
  if (wspeed>=10.8 && wspeed<13.9){beauf= 6;}
  if (wspeed>=13.9 && wspeed<17.2){beauf= 7;}
  if (wspeed>=17.2 && wspeed<20.8){beauf= 8;}
  if (wspeed>=20.8 && wspeed<24.5){beauf= 9;}
  if (wspeed>=24.5 && wspeed<28.5){beauf=10;}
  if (wspeed>=28.5 && wspeed<32.7){beauf=11;}
  if (wspeed> 32.7)               {beauf=12;}
  if (debug==true){Serial.print  ("Beaufort");Serial.println(beauf);}
}

void readvalues(){
EEPROM.begin(50);
Serial.println("Reading stored values from eeprom");  
delay(500);
  Ftemp       = EEPROM.readString(0 );
  Fhum        = EEPROM.readString(5 );
  Fbar        = EEPROM.readString(10);
  Fsoil       = EEPROM.readString(15);
  Ftemp2      = EEPROM.readString(18);
  Fhum2       = EEPROM.readString(23);
  Fsoil2      = EEPROM.readString(28);
  TMinutesS1  = EEPROM.readInt   (31);
  TMinutesS2  = EEPROM.readInt   (35);
  TMinutesIW  = EEPROM.readInt   (39);
  
  Serial.print  ("Ftemp:");
  Serial.println (Ftemp);
  Serial.print  ("Fhum :");
  Serial.println (Fhum);
  Serial.print  ("Fbar :");
  Serial.println (Fbar);
  Serial.print  ("Fsoil:");
  Serial.println (Fsoil);
  Serial.print  ("Ftemp2:");
  Serial.println (Ftemp2);
  Serial.print  ("Fhum2 :");
  Serial.println (Fhum2);
  Serial.print  ("Fsoil2:");
  Serial.println (Fsoil2);
  Serial.print  ("TMinutesS1:");
  Serial.println (TMinutesS1);
  Serial.print  ("TMinutesS2:");
  Serial.println (TMinutesS2);
  Serial.print  ("TMinutesIW:");
  Serial.println (TMinutesIW);
}

void Sleep(){
  int address = 0;
  EEPROM.begin(50);
  Serial.println("storing last readings to eeprom");  
  delay(500);
  EEPROM.writeString(address, Ftemp);
  Serial.print  (address);
  Serial.print  (":");
  Serial.println(Ftemp);
  address += Ftemp.length() + 1;
    
  EEPROM.writeString(address,  Fhum);
  Serial.print  (address);
  Serial.print  (":");
  Serial.println(Fhum);
  address += Fhum.length() + 1;
    
  if (Fbar.length()==3){Fbar="0"+Fbar;}

  EEPROM.writeString(address,  Fbar);
  Serial.print  (address);
  Serial.print  (":");
  Serial.println(Fbar);
  address += Fbar.length() + 1;
    
  EEPROM.writeString(address,  Fsoil);
  Serial.print  (address);
  Serial.print  (":");
  Serial.println(Fsoil);
  address += Fsoil.length() + 1;
    
  EEPROM.writeString(address,  Ftemp2);
  Serial.print  (address);
  Serial.print  (":");
  Serial.println(Ftemp2);
  address += Ftemp2.length() + 1;
    
  EEPROM.writeString(address,  Fhum2);
  Serial.print  (address);
  Serial.print  (":");
  Serial.println(Fhum2);
  address += Fhum2.length() + 1;
    
  EEPROM.writeString(address,  Fsoil2);
  Serial.print  (address);
  Serial.print  (":");
  Serial.println(Fsoil2);
  address += Fsoil2.length() + 1;

  EEPROM.writeInt(address,  TMinutesS1);
  Serial.print  (address);
  Serial.print  (":");
  Serial.println(TMinutesS1);
  address += sizeof(int);

  EEPROM.writeInt(address,  TMinutesS2);
  Serial.print  (address);
  Serial.print  (":");
  Serial.println(TMinutesS2);
  address += sizeof(int);

  EEPROM.writeInt(address,  TMinutesIW);
  Serial.print  (address);
  Serial.print  (":");
  Serial.println(TMinutesIW);
  
  EEPROM.commit();

delay(500);
esp_sleep_enable_ext0_wakeup(GPIO_NUM_35,0); //1 = High, 0 = Low
esp_deep_sleep_start();
}

void CheckSensorServers(int server){//0=both 1=1 2-=2 verbose option?
  if (server==0||server==1){
Serial.print("Pinging Server1:");
    bool Sensor1_UP = Ping.ping("192.168.2.10",3);//Sensor1Server
//  bool Sensor1_UP = Ping.ping(IPServer1,3);
    if (!Sensor1_UP) {Serial.println("Sensor1 down")  ;sensor1kleur=TFT_RED  ;tft.setTextColor(sensor1kleur,TFT_BLACK);tft.println("Sensor1 down"  );sensor1serverup=false;}
    else             {Serial.println("Sensor1 online");sensor1kleur=TFT_GREEN;tft.setTextColor(sensor1kleur,TFT_BLACK);tft.println("Sensor1 online");sensor1serverup=true;} 
    if (debug==true) {Serial.println("Ping finished");}
  }
  if (server==0||server==2){
Serial.print("Pinging Server2:");
    bool Sensor2_UP = Ping.ping("192.168.2.11",3);//Sensor2Server
//  bool Sensor2_UP = Ping.ping(IPServer2,3);
    if (!Sensor2_UP) {Serial.println("Sensor2 down")  ;sensor2kleur=TFT_RED  ;tft.setTextColor(sensor2kleur,TFT_BLACK);tft.println("Sensor2 down"  );sensor2serverup=false;}
    else             {Serial.println("Sensor2 online");sensor2kleur=TFT_GREEN;tft.setTextColor(sensor2kleur,TFT_BLACK);tft.println("Sensor2 online");sensor2serverup=true;}    
    if (debug==true){Serial.println("Ping finished");}
  }
  tft.setTextColor (TFT_WHITE,TFT_BLACK);
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
