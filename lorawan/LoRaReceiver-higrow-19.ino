/* \Arduino\LORAWAN\TTGO-LoRa-Series-master\LoRaReceiver\LoRaReceiver-higrow-19 to use with
 * \Arduino\LORAWAN\TTGO-LoRa-Series-master\LoRaSender\LoRaSender-higrow-10.9
 * for (ESP32 Dev Module):LilyGO TTGO T3 LoRa32 868MHz V2.1.6 ESP32
 * reads text records from lorasenders higrow
 */
#define OLDLORA    1            // SET! when using 1.0 board io. 2.14
#define SND_MSG    1            // SET! confirmation message
#define USE_U8G2   1            // SET! use graphic signs io. text
#define RST_ESP    1            // SET! restart ESP every 60 min to avoid hanging(set value interval below)
#define CHK_RSSI   1            // SET! display last RSSI and SNR
#define CHK_HEAP   1            // SET! check memory heap
#define USE_WIFI   1            // SET! use WIFI to get timestamp
#define USE_EEPR   1            // SET! store la  st read values in eeprom
#define USE_BLYNK  1            // SET! use of Blynk dashboard
#define USE_RELAIS 1            // SET! version with relais and buttons
#define SND_MSGX   0            // SET! extra confirmation message
#define DEBUG      0            // SET! dump incoming string to screen or just their values
#define CHK_XTRA   0            // SET! check first byte of message is yours
#define CHK_LORA   0            // SET! check if lora.receive still active
#define USE_INSIDE 0            // SET! reduce radiosignal when testing
#define USE_SYNCW  0            // SET! use syncword to avoid stay messages (set in sender sketch too)
#define MIDNIGHT   0            // SET! restart ESP at midnight
#define NrSensors  2            // SET!
#define BUT_IN     0            // onboard button to turn screen off
#define LED_IN     2            // onboard led indicates water

#if USE_RELAIS                  // note! pinout of 1.0 and 2.0 board is different but both have these pins
  #define         relayPin  14
  #define         lbutton   12
  #define         rbutton   13 
  bool            buttons;       // are there buttons attached to set threshold? 
  bool            water   =false; 
  bool            oldwater=false; 
#endif

#if   USE_BLYNK
/*
 * https://blynk.cloud/dashboard  create a template with:
 * 
 * Id Name          Pin Datatype  Units min max def
 * 1  time1s        V0  String  
 * 2  time1s        V1  String  
 * 3  Thresholdi    V2  Integer         0   100   0
 * 4  Statuss       V3  String
 * 5  FrogardenperciV4  Integer    %    0   100   0
 * 6  BckgardenperciV5  Integer    %    0   100   0
 * 7  msg1ins1      V6  String
 * 8  msg1ins2      V7  String
 * 9  avghumi       V8  Integer         0   100   0
 * 10 wateri        V9  Integer         0   1     0
 * 11 errors1i      V10 Integer         0   32767 0
 * 12 errors2i      V11 Integer         0   32767 0
 * 13 running1i     V12 Integer    d    0   999   0
 * 14 running2i     V13 Integer    d    0   999   0
 * 15 S1OK          V14 Integer         0   1     0
 * 16 S2OK          V15 Integer         0   1     0
 * 17 Heartbeat     V16 Integer         0   1     0
 * 18 Since1i       V17 Integer         0   999   0
 * 19 Since2i       V18 Integer         0   999   0
 * 20 Thresholdseti V19 Integer         0   100   15
 * 21 RSSI1s        V20 Stringr                    
 * 22 RSSI2s        V21 Stringr                    
 * 23 Switch        V22 Integer         0   1    
 */ 
  #define BLYNK_PRINT Serial
  #if OLDLORA     // I have 2 different boards they use different template                             
    #define BLYNK_TEMPLATE_ID "TMPL878asd6f"                      // SET!
    #define BLYNK_DEVICE_NAME "mygarden"                          // SET!
    #define BLYNK_AUTH_TOKEN  "TKM9cdkslfji43566j6836j4654Wyzp8"  // SET!
  #else
    #define BLYNK_TEMPLATE_ID "TMPLklfdsj6Y"
    #define BLYNK_DEVICE_NAME "myDash"
    #define BLYNK_AUTH_TOKEN  "igpogiafgmkJHJYqwetHupoiflcdxkmU"
  #endif
  #include <BlynkSimpleEsp32.h>
  bool  flip;                     // for heartbeat led blynk
  char  auth[] = BLYNK_AUTH_TOKEN;
  int       TsinceS1;             // minutes gone since last reading sensor 1
  int       TsinceS2;             // minutes gone since last reading sensor 2
  BlynkTimer timer;
  WidgetLED led1(V9);
  WidgetLED led2(V14);
  WidgetLED led3(V15);
  WidgetLED led4(V16);
  #if !defined USE_WIFI
    #define    USE_WIFI
  #endif
#endif

#if  RST_ESP
//  long interval     = 3600000; // SET! interval  60 minutes to restart ESP every 60 min to avoid hanging
  long interval       = 7200000; // SET! interval 120 minutes to restart ESP every 60 min to avoid hanging
  long previousMillis =  millis  ();
#endif

#if  CHK_LORA
  long interval2       =  450000; // SET! interval 7.5 minutes to check lora(band) to avoid hanging
  long previousMillis2 =  millis  ();
#endif

#if  CHK_RSSI
  int snr;
  int rsi;
  int snrv [NrSensors]={0,0};
  int rsiv [NrSensors]={0,0};
#endif

#if  CHK_HEAP
  #define HEAP_DIF  10000        // SET! reset when memory loss > HEAP_DIF
#endif

#if  OLDLORA
  #include "lora1.0_board_def.h" // for 1.0  board with seperate antenna
#else  
  #include "lora2.0_board_def.h" // for 2.14 board with onboard  antenna
#endif

#if  USE_WIFI
  #include <ArduinoJson.h>
  #include <NTPClient.h>
  #include <WiFi.h>
  #include <WiFiUdp.h>
  #include <WiFiClient.h>
  #include "my_secrets.h"                       //or use 2 lines below
  //#define WIFI_SSID       "yourWIFISSID"      // ssid
  //#define WIFI_PASSWORD   "yourWIFIPassword"  // password
  //RTC
  StaticJsonDocument<1000> doc;
  WiFiUDP   ntpUDP;                // Define NTP Client to get time
  NTPClient timeClient(ntpUDP);    // Variables to save date and time
#endif

#include <SPI.h>
#include <LoRa.h>

#if USE_EEPR
  #include "EEPROM.h"
  String EepSave [NrSensors]={"11:11MYDATA01113500200020","11:11MYDATA01113500200020"};//25 long
#endif

String Sensors [NrSensors]={"S99 99% 999 errors.","S99 99% 999 errors."}; //20 long
String SensMes [NrSensors]={"?????","?????"};   //5 long
String SensTim [NrSensors]={"wait!","wait!"};   //5 long
int    NrMsgs  [NrSensors]={0,0};
int    Humi    [NrSensors]={33,33};
int    Temp    [NrSensors]={0,0};//temp or errorcounter
int    count    = 0;              // total nr of correct received packages
int    expLEN   = 20;             // SET! length of "MYDATA01102500100000"
int    flop     = 0;              // for heartbeat oled
int    mytimer  = 0;
String ID       = "MYDATA";       // SET! your chosen identifier
String expID    = ID;         
// choose your own coding/decoding strings but use same strings as in sender program
String normal   = "ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890+-";//SET! decode
String coded    = "1234567890abcdefghijklmnopqrstuvwxyz-+";//SET! code (pa A becomes 1)
String msgin    = "MYDATA01102509900000";
String msgout   = msgin;
String recv     = msgin;
String subst    = "X";
String LoRaStat = "*";
String myMessage=msgin;
String sentence=msgin;
String SN="99";                   // sensor number
String DN="9";                    // destination number
String TT="999";                  // temp reading(3)
String HH=TT;                     // hum reading (3)
String MN="99999";                // messagenumber (5)
int    T;                         // temp   reading  as integer
int    H;                         // hum    reading  as integer
int    S;                         // sensor  number as integer
int    M;                         // message number as integer
int    Threshold  = 15;
int    oldThreshold  = Threshold;
long   freememstart;  
String formattedDate="2018-05-28T16:12:34Z";
String TimeStamp="99:99";
bool   LCDon=true;
String msg;
String STHours;
String STMinutes;
int    TMinutes;
int    PrevMins[NrSensors]={0,0};
int    gemhum=15;

#if USE_U8G2
  #include <U8g2lib.h>
  // fontlists at https://github.com/olikraus/u8g2/wiki/fntlistall
  #if OLDLORA 
      U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, OLED_SCL, OLED_SDA , OLED_RST);
  #else
      U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, OLED_SCL, OLED_SDA , U8X8_PIN_NONE);
  #endif  
#else
  OLED_CLASS_OBJ display(OLED_ADDRESS, OLED_SDA, OLED_SCL);
#endif

#if  USE_BLYNK
  // This function will be called every time Slider Widget
  // in Blynk app writes values to the Virtual Pin V19
  BLYNK_WRITE(V19)
  {
  int pinValue = param.asInt(); // assigning incoming value from pin V19 to a variable
  // process received value
  Threshold = pinValue;
  Blynk.virtualWrite  (V2, Threshold);
  //Blynk.virtualWrite  (V19,Threshold);
  Serial.println    ("*blynk slider");
  }

  BLYNK_WRITE(V22)// reset switch
  {
  int pinValue = param.asInt(); // assigning incoming value from pin V19 to a variable
  // process received value
  Serial.println    ("*blynk switch");
  resettime();
  }
  
  // This function is called every time the device is connected to the Blynk.Cloud
  BLYNK_CONNECTED()
  {
      Blynk.virtualWrite  (V3 ,"Verbonden met Blynk");
      Blynk.virtualWrite  (V2, Threshold);
      Serial.println      ("Blynk_Connect");
      led1.off  ();
      led2.off  ();
      led3.off  ();
      led4.on   ();
  }
  // This function sends ESP32 uptime every 10 seconds (see timer.setInterval) to some virtual Pins
  void myTimerEvent()
  {   
//    String msg;
    gemhum =(Humi[0]+ Humi[1])/2;
    if (gemhum<Threshold){water=true;led1.on();msg="ga sproeien!"; 
    } else {water=false;led1.off();msg="nat genoeg!";}
    digitalWrite(LED_IN,water);
    if (flip){led4.on();}else{led4.off();}flip=!flip; 
    Blynk.virtualWrite  (V3,  msg);
    Blynk.virtualWrite  (V8,  gemhum);
    Blynk.virtualWrite  (V2,  Threshold);
    Blynk.virtualWrite  (V5,  Humi   [1]);
    Blynk.virtualWrite  (V4,  Humi   [0]);
  }
#endif
      
// ########################### setup ###########################
void setup()
{
    LCDon=true;
    Serial.begin               (115200);
    while                      (!Serial);
    delay                      (1000);  
    Serial.println             (__FILE__);
    Serial.println             ("Config setup:");
    #if DEBUG            
      Serial.println           ("## debug mode set ##");         
    #endif
    #if OLDLORA             
      Serial.println           ("-LoRa board v1.0");            
    #else                      
      Serial.println           ("-LoRa board v2.x");            
    #endif
    #if SND_MSG
      Serial.println           ("-send confirmation message");         
    #endif
    #if CHK_XTRA            
      Serial.println           ("-send extra message");         
    #endif
    #if USE_SYNCW
      Serial.println           ("-use syncword:");         
//    Serial.println           (sword);         
    #endif
    #if CHK_RSSI            
      Serial.println           ("-check signal strength");      
    #endif
    #if CHK_HEAP            
      Serial.println           ("-check memory heap");          
    #endif
    #if USE_INSIDE          
      Serial.println           ("-reduce radiosignal");         
    #endif
    #if USE_WIFI            
      Serial.println           ("-use WIFI for rtc or Blynk");           
    #endif
    #if  USE_BLYNK
      Serial.println           ("-use Blynk");           
    #endif
    #if USE_EEPR            
      Serial.println           ("-use Eeprom to store values"); 
    #endif
    #if  RST_ESP
      Serial.print             ("-restarting ESP for stability every "); 
      Serial.print             (interval/60000) ;Serial.println(" minutes");
    #endif
    #if  CHK_LORA
      Serial.print             ("-checking lora.receive for stability every "); 
      Serial.println           (interval2/60000);Serial.println(" minutes");
    #endif
    #if USE_RELAIS            
      Serial.println           ("-use relais and buttons on pins 12,13,14:"); 
      pinMode                  (lbutton, INPUT);
      pinMode                  (rbutton, INPUT);
      pinMode                  (relayPin,OUTPUT);
      digitalWrite             (relayPin,LOW);
      if (digitalRead          (lbutton)==true){buttons=false;Serial.print("NO buttons found");} else {buttons=true;Serial.print("buttons found");}// no button attached on 8266
    #endif

    if (OLED_RST > 0) {
        pinMode                (OLED_RST, OUTPUT);
        digitalWrite           (OLED_RST, HIGH);
        delay                  (100);
        digitalWrite           (OLED_RST, LOW);
        delay                  (100);
        digitalWrite           (OLED_RST, HIGH);
    }

    pinMode                    (BUT_IN, INPUT);
    pinMode                    (LED_IN, OUTPUT);
  
    String temp =   __FILE__;
    int       t = temp.length  ();
    msg         = "Start";

    #if USE_U8G2
      u8g2.begin               ();
      u8g2.setContrast         (3);            // x from 0 to 255 works, but do not disable oled. value x=0 still display some data..
      u8g2.clearBuffer         ();
      u8g2.setFont             (u8g2_font_courB10_tf);   // 16h https://github.com/olikraus/u8g2/wiki/fntgrpadobex11#courb10
      u8g2.setCursor           (0,16);
      u8g2.print               (temp.substring(t-18,t-3)); 
      u8g2.sendBuffer          ();
    #else
      display.init             ();
      display.flipScreenVertically();
      display.setTextAlignment (TEXT_ALIGN_LEFT);
      display.setFont          (ArialMT_Plain_16); // ArialMT_Plain_24 ArialMT_Plain_10
      display.clear            ();
      display.drawString       (0 ,0 , msg);
      display.drawString       (0 ,16, temp.substring(t-32,t-16));
      display.drawString       (0 ,32, temp.substring(t-16,t));
    #endif
    
    #if  USE_WIFI
      msg =                    "WiFi";
      WiFi.begin               (ssid,password);
      Serial.print             (msg);
      #if USE_U8G2
        u8g2.setCursor         (0,32);
        u8g2.print             (msg); 
        u8g2.sendBuffer        ();
      #else
        display.clear          ();
        Serial.print           (msg);
        display.drawString     (64,16,msg);
        display.display        ();
      #endif     
      int wc=0;
      while (WiFi.status() != WL_CONNECTED) {
        wc++;
        if (wc>20){esp_restart();}// after 10 secs
        Serial.print(wc);
        #if USE_U8G2
          u8g2.print            ("."); 
          u8g2.sendBuffer       ();
        #endif
        delay(500);
      }

      msg = "Connected : ";
      Serial.print             (msg);
      Serial.println           (WiFi.SSID());
      Serial.print             ("IP:");
      Serial.println           (WiFi.localIP().toString());
      msg = WiFi.localIP().toString();
      #if USE_U8G2
        u8g2.setCursor         (0,48);
        u8g2.print             (msg); 
        u8g2.sendBuffer        ();    
      #else
        display.drawString     (0,48, "IP:" + msg);
        display.display        ();
      #endif
    #else
      msg = "not using WIFI";
      #if USE_U8G2
        u8g2.setCursor         (0,48);
        u8g2.print             (msg); 
        u8g2.sendBuffer        ();    
      #else
        display.drawString     (0,48,msg);
        display.display        ();
      #endif
    #endif
    delay(2500);               // show text on display
     
    SPI.begin                  (CONFIG_CLK, CONFIG_MISO, CONFIG_MOSI, CONFIG_NSS);
    LoRa.setPins               (CONFIG_NSS, CONFIG_RST, CONFIG_DIO0);

    msg = "LoRa fail!";
    while (!LoRa.begin(BAND)){
        Serial.println         (msg);
        #if USE_U8G2
          u8g2.setCursor       (0,64);
          u8g2.print           (msg); 
          u8g2.sendBuffer      ();    
        #else
          display.drawString   (0,64, msg);
          display.display      ();
        #endif
        delay                  (1000);
    }
 
    msg = "LoRa UP!";
    Serial.println             (msg);
    #if USE_U8G2
        u8g2.setCursor         (0,64);
        u8g2.print             (msg); 
        u8g2.sendBuffer        ();    
    #else
        display.drawString     (0,64,msg);
        display.display        ();
    #endif

    #if USE_INSIDE
      LoRa.setTxPower(3);      // txPower - TX power in dB, defaults to 17
    #else                      // Supported values are 2 to 20
      LoRa.setTxPower(8);
    #endif
    
    #if USE_SYNCW
      //##5 Change sync word (0xBA) to match the receiver
      // The sync word assures you don't get LoRa messages from other LoRa transceivers
      // ranges from 0-0xFF
      LoRa.setSyncWord        (0xBA);//SET! if used
    #endif

    #if  USE_WIFI
      //RTC Initialize a NTPClient to get time
      timeClient.begin        (); 
      timeClient.setTimeOffset(7200);  // NOT DST 3600
      while                   (!timeClient.update()) {timeClient.forceUpdate();}
    #endif
    
    LoRa.receive              ();
    freememstart=esp_get_free_heap_size();
    Serial.print              ("##>esp_get_free_heap_size():");  
    Serial.println            (freememstart);  

    #if  USE_BLYNK
      Blynk.begin(auth, ssid, password, "blynk.cloud", 8080);
      // Setup a function to be called every second
      // timer.setInterval(1000L, myTimerEvent);
         timer.setInterval( 5000L, myTimerEvent);// every  5 seconds
      // timer.setInterval(10000L, myTimerEvent);// every 10 seconds
      // timer.setInterval(30000L, myTimerEvent);// every 30 seconds
      // timer.setInterval(60000L, myTimerEvent);// every minute
    #endif 

    #if USE_EEPR
      ReadFromEeprom          ();
      SetLeds                 ();
    #endif 

}

// ########################### LOOP  ###########################

void loop(){

  unsigned long currentMillis = millis(); 
  flop++;                     //display heartbeat on oled
  if (flop>50000){            // 5   secs
    flop=0;
    if (LoRaStat=="+"){LoRaStat="x";} else {LoRaStat="+";}
    displayOled();

    #if  USE_BLYNK
    mytimer++;
    if (mytimer==10){          // prox 1 min set leds and blynk values
        mytimer=0;
        SetLeds();
      }      
    #endif 
  }

  #if USE_RELAIS  
    if (Threshold!=oldThreshold){displayThresh();ThresholdToEeprom();oldThreshold=Threshold;}
    if (water && !oldwater){        // water has become true turn on relay
      digitalWrite(relayPin,HIGH);
      oldwater=true;
      Serial.print       ("water has become true  turn on relay ");
      Serial.print       (gemhum); 
      Serial.print       ("/");
      Serial.println     (Threshold);
    }        
    if (!water && oldwater){        // water has become false turn off relay
      digitalWrite(relayPin,LOW);
      oldwater=false;    
      Serial.print       ("water has become false turn off relay ");
      Serial.print       (gemhum); 
      Serial.print       ("/");
      Serial.println     (Threshold);
    }   
  #endif
  
  #if  USE_BLYNK
    Blynk.run();
    timer.run();
  #endif
  
  #if  CHK_LORA
    if   (currentMillis - previousMillis2 > interval2) {
        if (!LoRa.begin(BAND)){
          Serial.println      ("Checking LoRa failed! Restarting ...");
          delay               (500);      
          ESP.restart         ();    
        }    
        Serial.println        ("LoRa OK, resetting looptimer");
        LoRaStat        =     "+";
        previousMillis2 =     currentMillis;
    }
  #endif

  #if  RST_ESP
    if   (currentMillis - previousMillis > interval) { 
      Serial.println        ("restarting ESP after interval.");
      delay                 (500);      
      ESP.restart           ();
    }
  #endif

  int packetSize = LoRa.parsePacket();
  if (packetSize) {  
      recv = "";
//    while (LoRa.available()) {recv += (char)LoRa.read();} ## replace
      while (LoRa.available()) {recv  = LoRa.readString();}

      #if  CHK_RSSI
        snr= LoRa.packetSnr ();
        rsi= LoRa.packetRssi();
        Serial.print        ("RSSI/SNR:");      
        Serial.print        (rsi); 
        Serial.print        ("/"); 
        Serial.println      (snr); 
      #endif
      
      int lenIN   =       recv.length();      // length of "MYDATA01100200100047"
      Serial.print        ("received package long ");      
      Serial.println      (lenIN); 

      if                  (lenIN==expLEN){
        #if CHK_XTRA
          //XTRA CHK 2 PREVENT HANGING ESP
          String CHK = recv.substring(0,1); //"c"
          int p = coded.indexOf     (CHK);  //12
          subst = normal.substring  (p,p+1);//"M"
          #if DEBUG
              Serial.print      (CHK);   
              Serial.print      (p);   
              Serial.println    (subst);           
          #endif
          if (expID.substring(0,1)!=subst){ID="X";} // extra check: 1st character of message is as expected
        #endif 
                 
        if (ID!="X"){
          codit      (recv,false);        // now convert coded msgin to msgout
          recv    =   msgout;
          ID      =   recv.substring(0,6);// "MYDATA"          
          count++;
        }
          
        if (ID==expID){
          //"MYDATA01102509900000"
          breakdown(recv);                // get vars out of message

          // now send conformation message
          // identifier send to sender 0? from receiver 1 = me received OK
          // myMessage ="MYDATA0?1OK";
        
          myMessage =expID;               //"MYDATA"
          myMessage+=SN;                  // 01 or 02
          myMessage+=DN;                  // former destination = 1 = me
          myMessage+="OK";                // "MYDATA011OK"         
          codit(myMessage,true);          // convert msgin to coded msgout
          myMessage=msgout;

          #if SND_MSG
            Serial.print        ("sending  :");   
            Serial.println      (myMessage);  
            LoRa.beginPacket    ();
            LoRa.print          (myMessage);
            LoRa.endPacket      ();
            //2testlater        LoRa.endPacket(true); // true = async / non-blocking mode
            //https://www.rfwireless-world.com/Terminology/What-is-difference-between-Implicit-header-and-Explicit-header-in-LoRaWAN-packet.html
          #else
            Serial.println      ("Not sending confirmation message.");
          #endif

          #if USE_WIFI
            formattedDate   =   timeClient.getFormattedDate();
            int pos         =   formattedDate.indexOf("T");
            TimeStamp       =   formattedDate.substring(pos+1, formattedDate.length()-1-3);
          #else
            TimeStamp       =   "99:99";
            delay (250);
          #endif

          S = SN.toInt  ();      
          ParseReadings (S,true);// store data in arrays and update counter                      
          LoRaStat  =   "R";
          
          #if DEBUG
            displayDebug();
          #else
            displayOled();          
          #endif

          #if SND_MSGX        //SEND AGAIN 2 BE SURE        
            delay             (1000); 
            Serial.print      ("sending  :");   
            Serial.println    (myMessage);  
            LoRa.beginPacket  ();
            LoRa.print        (myMessage);
            LoRa.endPacket    ();
          #else
            delay (50);
          #endif

          #if DEBUG
            displayDebug();
          #else
            displayOled();          
          #endif

          #if USE_EEPR
            WriteToEeprom       ();         // update eeprom values for reset or restart
          #endif

          #if  USE_BLYNK
              GetMinutes       (TimeStamp);
              PrevMins [S-1] = TMinutes;             
              M=SensMes[S-1].toInt();

              if (S==1){
                led2.off();
                Blynk.virtualWrite(V0 ,SensTim[0]);
                Blynk.virtualWrite(V4 ,Humi   [0]);
                Blynk.virtualWrite(V6 ,SensMes[0]);
                Blynk.virtualWrite(V10,Temp   [0]);
                Blynk.virtualWrite(V12,String(M/24));                                 //days  running 24hpd
                Blynk.virtualWrite(V20,String(snrv[0])+"/"+String(rsiv[0]));          //signal strength
              } else {
                led3.off();
                Blynk.virtualWrite(V1 ,SensTim[1]);
                Blynk.virtualWrite(V5 ,Humi   [1]);
                Blynk.virtualWrite(V7 ,SensMes[1]);
                Blynk.virtualWrite(V11,Temp   [1]);
                Blynk.virtualWrite(V13,String(M/24));                                 //days  running 24hpd
                Blynk.virtualWrite(V21,String(snrv[1])+"/"+String(rsiv[1]));          //signal strength
              }
          #endif

          #if SND_MSG
            delay(1000);LoRa.receive();                                               // after sending start receiving again
          #else
            LoRaStat="I";
            displayOled();                                                            // end of some packet same length
          #endif 
         
      } else { // not expected ID
          Serial.println  ("Package is not mine ... wrong ID ignoring package");         
      }
    } // end of len = expexted len
    else   {Serial.println("Package is not mine ... wrong length ignoring package");} // end of some packet
    
    #if CHK_HEAP //##4
      Serial.print    ("*ESP.getFreeHeap():");
      Serial.print    (ESP.getFreeHeap());
      Serial.print    (" esp_get_free_heap_size():");  
      Serial.print    (esp_get_free_heap_size());
      Serial.print    (" DIFF:");
      int freeheap = esp_get_free_heap_size()-freememstart;
      Serial.println  (freeheap);
      if              (freeheap<-HEAP_DIF){
          Serial.println          ("restarting ESP because free memory is getting low.");
          delay                   (1000);      
          ESP.restart             ();   
      } // end of small heapsize
      #endif
    }  // end of parsepacket
  #if USE_RELAIS
     checkbuttons();
  #endif
}      // end of loop

// ########################### VOIDS ###########################

void codit(String secretmessage, bool mydir){ // mydir true => encrypt false => decrypt message
  Serial.print      ("*codit(");
  Serial.print      (secretmessage);
  Serial.print      (",");
  Serial.print      (mydir);
  Serial.println    (")");
  msgout      =     "";
  Serial.print      ("secret   :");
  Serial.println    (secretmessage);
  for (int i = 0; i < secretmessage.length(); i++) {
      String x      = secretmessage.substring(i,i+1);
      if (mydir){
        int y = normal.indexOf   (x);
        if (y>normal.length()){subst="*";Serial.print("not found char:");Serial.println(x);}
        else {subst = coded.substring   (y,y+1);}            
      } else {
        int y = coded.indexOf    (x);
        if (y>coded.length()) {subst="*";Serial.print("not found char:");Serial.println(x);}
        else {subst = normal.substring (y,y+1);}
      }
      msgout += subst;
  }
  Serial.print          ("converted:");
  Serial.println        (msgout);
  if (secretmessage.length()!=msgout.length()){
    Serial.println      ("Error: output string wrong length!");
    msgout =            secretmessage;
  }
}

#if USE_EEPR
  void WriteToEeprom (){
      Serial.println    ("*WriteToEeprom()");
      EEPROM.begin      (64); 
      //11:11MYDATA0111350020002012:12MYDATA02113500200020" sample format of message
      sentence        = SensTim[0]+EepSave[0]+SensTim[1]+EepSave[1];
      if (sentence.length()>64){ // error in message do not overwrite eeprom 2 be sure not 2 hangup esp
          sentence="12:129999999999999999999912:1299999999999999999999";
      }
      EEPROM.writeString(0, sentence);
      EEPROM.writeInt   (60,Threshold); 
      EEPROM.commit     ();
      #if DEBUG
        Serial.print    ("Written to eeprom:");
        Serial.print    (sentence);
        Serial.print    (" long:");
        Serial.println  (sentence.length());
        Serial.print    ("Written Threshold:");
        Serial.println  (Threshold);      
      #endif
  } 

  void ThresholdToEeprom(){
      Serial.println    ("*ThresholdToEeprom()");
      EEPROM.begin      (64); 
      EEPROM.writeInt   (60,Threshold); 
      Serial.print      ("Written Threshold:");
      Serial.println    (Threshold);
      EEPROM.commit     ();    
  }
  
  void ReadFromEeprom   (){
      Serial.println    ("*ReadFromEeprom()");
      EEPROM.begin      (64); 
      sentence  = EEPROM.readString (0);
      #if DEBUG
        Serial.print      ("Read from eeprom:");
        Serial.print      (sentence);
        Serial.print      (" long:");
        Serial.println    (sentence.length());
        Threshold = EEPROM.readInt(60);
        Serial.print      ("Read Threshold:");
        Serial.println    (Threshold);
      #endif
      //1st time
      if (Threshold<0||Threshold>99){
        Threshold=15;
        Serial.println  ("Resetting Threshold to 15");
        EEPROM.writeInt (60,Threshold);
        EEPROM.commit   (); 
      }
      //11:11MYDATA0111350020002012:12MYDATA02113500200020"
      SensTim [0] =     sentence.substring(0    ,5      );// "11:11"
      SensTim [1] =     sentence.substring(0+25 ,0+25+5 );// "12:12"
      EepSave [0] =     sentence.substring(5    ,5+20   );// "MYDATA01113500200020"
      EepSave [1] =     sentence.substring(5+25 ,5+25+20);// "MYDATA02113500200020"
      Serial.println    ("Interpreting latest packages read from eeprom ...");       
      recv        =     EepSave[0];
      breakdown         (EepSave[0]);
      TimeStamp   =     SensTim [0];
      GetMinutes        (TimeStamp);
      PrevMins[0] =     TMinutes;
      ParseReadings     (1,true);// RECV=FOUT
      recv        =     EepSave[1];
      breakdown         (EepSave[1]);
      TimeStamp   =     SensTim [1];
      GetMinutes        (TimeStamp);
      PrevMins[1] =     TMinutes;
      ParseReadings     (2,true);
      displayOled       ();
      #if  USE_BLYNK
        SetLeds         ();
      #endif
  }
#endif

void breakdown(String input){
  Serial.print      ("*breakdown(");
  Serial.print      (input);
  Serial.println    (")");
  ID  = input.substring(0 , 6);   //"MYDATA"
  SN  = input.substring(6 , 8);   //"01";     sensor number
  DN  = input.substring(8 , 9);   //"1";      destination number
  TT  = input.substring(9 ,12);   //"025";    temp reading(3)
  HH  = input.substring(12,15);   //"099";    hum reading (3)
  MN  = input.substring(15,20);   //"01234";  messagenumber
  T   = TT.toInt();               //25 
  TT  = String(T);
  H   = HH.toInt();               //99
  HH  = String(H);
  S   = SN.toInt();               //1       
  M   = MN.toInt();               //1234  
  #if DEBUG
    Serial.print      ("total received packages:");
    Serial.print      (count);
    Serial.print      (" this one is ");
    Serial.print      (M);
    Serial.print      (" from sensor ");
    Serial.println    (S);
    Serial.print      ("*****message->");
    Serial.print      (input);
    Serial.println    ("<- end of message"); 
    Serial.print      ("ID:");
    Serial.println    (ID);
    Serial.print      ("SN:");
    Serial.println    (SN);
    Serial.print      ("DN:"); 
    Serial.println    (DN);
    Serial.print      ("TT:");
    Serial.println    (TT);
    Serial.print      ("HH:");
    Serial.println    (HH);
    Serial.print      ("MN:");
    Serial.println    (MN);
  #endif
}

void ParseReadings(int tsensor,bool updmsg){// sensor nr 1 or 2 stored in aray[0] or array[1]
     Serial.print      ("*ParseReadings(");
     Serial.print      (tsensor);
     Serial.print      (",");
     Serial.print      (updmsg);
     Serial.print      (") storing value recv=");
     Serial.println    (recv);
     
//   Sensors[tsensor-1]  = "S"+SN+" "+HH+"%"+" "+TT+" errors";
     Sensors[tsensor-1]  = "S"+SN+" "+HH+"%";
     SensMes[tsensor-1]  = MN;
     if (updmsg){
        SensTim [tsensor-1] = TimeStamp;
        EepSave [tsensor-1] = recv;
        rsiv    [tsensor-1] = rsi;
        snrv    [tsensor-1] = snr;
        Humi    [tsensor-1] = H;
        Temp    [tsensor-1] = T;
        NrMsgs  [tsensor-1]++;
     }
}

#if DEBUG
  void displayDebug(){                    // just dump received string on display
      String  PART1  = recv.substring(0,10);
      String  PART2  = recv.substring(10,20);
      #if USE_U8G2
        u8g2.clearBuffer   ();
        u8g2.setCursor     (0,32);
        u8g2.print         (PART1); 
        u8g2.setCursor     (0,64);
        u8g2.print         (PART2); 
        u8g2.sendBuffer    ();    
      #else
       display.clear();
       display.drawString  (0 ,0, "Incoming data");
       display.drawString  (0 ,16, PART1);
       display.drawString  (0 ,32, PART2);
       display.drawString  (0 ,48, "from S");
       display.drawString  (48,48, SN);
       display.display     ();
      #endif
  }
#endif

void displayOled(){
//     Serial.println    ("*displayOled()");
   #if USE_U8G2
   char myChar = 65;
   if (water){myChar=241;}else{myChar=124;}
    if (LCDon){
     u8g2.clearBuffer     ();    
     u8g2.setFont         (u8g2_font_open_iconic_all_4x_t);//32 pix high
     u8g2.setCursor       (0,32);
     u8g2.print           (myChar);
     u8g2.setCursor       (5,60);
     u8g2.setFont         (u8g2_font_10x20_tf);//20 pix high https://github.com/olikraus/u8g2/wiki/fntgrpx11
     u8g2.print           (Threshold);
     u8g2.setFont         (u8g2_font_open_iconic_all_4x_t);//32 pix high
     
     u8g2.setCursor       (96,32);
     if (TsinceS1<=60){
       u8g2.print         (char(120));//OK  
     } else {
       u8g2.print         (char(121));//NOT OK  
     }  
     u8g2.setCursor       (96,64);
     if (TsinceS2<=60){
       u8g2.print         (char(120));//OK  
     } else {
       u8g2.print         (char(121));//NOT OK  
     }  
     u8g2.setFont         (u8g2_font_courB24_tf);       
     
     if (LoRaStat=="x"){
      u8g2.setCursor       (33,32);
      u8g2.print           (Humi[0]);
      u8g2.print           ("%"); 
      u8g2.setCursor       (33,64);
      u8g2.print           (Humi[1]);
      u8g2.print           ("%");     
     } else {
      u8g2.setCursor       (40,32);
      u8g2.setFont         (u8g2_font_open_iconic_all_4x_t);//32 pix high
      if (digitalRead      (relayPin)){
         u8g2.print        (char(91));  // battery full as symbol for relay up
      } else {
         u8g2.print        (char(90));
      }
      u8g2.setFont         (u8g2_font_courB24_tf);       
      u8g2.setCursor       (33,64);
      u8g2.print           (gemhum);
      u8g2.print           ("%");     
     }
     u8g2.sendBuffer       ();
    }    
   #else
     display.clear        ();
     display.drawString   (0    ,0, SensTim [0]);               // timestamp of latest message
     display.drawString   (64   ,0, SensTim [1]);
     display.drawString   (48   ,0, LoRaStat);
     display.drawString   (0    ,16,Sensors [0]);               // formatted string with data sensor1
     display.drawString   (0    ,32,Sensors [1]);
     #if  CHK_RSSI
       display.drawString (64   ,16,String(snrv [0]));
       display.drawString (80   ,16,String(rsiv [0]));
       display.drawString (64   ,32,String(snrv [1]));
       display.drawString (80   ,32,String(rsiv [1]));
     #endif
     if (LoRaStat=="x"){
      display.drawString  (0    ,48,SensMes [0]);               // nr of sent messages according to sender(sensor)
      display.drawString  (64   ,48,SensMes [1]); 
     } else {
     #if  USE_BLYNK
        display.drawString  (0    ,48,String(TsinceS1));        // time since last reading
        display.drawString  (64   ,48,String(TsinceS2));
     #else 
        display.drawString  (0    ,48,String(Temp[0]));         // sensor temp or nr errors
        display.drawString  (64   ,48,String(Temp[1]));
     #endif  
     }                          
     display.display      ();
   #endif
}

void GetMinutes(String Stamp){
  STHours            =  Stamp.substring(0,2);                   //"12:38"
  STMinutes          =  Stamp.substring(3,5);
  int Thours         =  STHours.toInt  ();
  int Tmins          =  STMinutes.toInt();
  TMinutes           =  (Thours*60)+Tmins;
}

#if  USE_BLYNK
void resettime(){    // simulate last update is now
     Serial.println    ("*resettime()");
     formattedDate   =  timeClient.getFormattedDate();
     int pos         =  formattedDate.indexOf("T");
     TimeStamp       =  formattedDate.substring(pos+1, formattedDate.length()-1-3);  
     GetMinutes         (TimeStamp);
     SensTim [0]     =  TimeStamp;
     SensTim [1]     =  TimeStamp;
     PrevMins[0]     =  TMinutes;     
     PrevMins[1]     =  TMinutes;     
     TsinceS1        =  0;
     TsinceS2        =  0;
     WriteToEeprom();
     SetLeds();
}
void SetLeds(){ 
//     Serial.println     ("*SetLeds()");
     formattedDate   =    timeClient.getFormattedDate();
     int pos         =    formattedDate.indexOf("T");
     TimeStamp       =    formattedDate.substring(pos+1, formattedDate.length()-1-3);
     GetMinutes           (TimeStamp);
     TsinceS1=TMinutes-PrevMins[0]; 
     TsinceS2=TMinutes-PrevMins[1]; 
     // check if timestamp is todays
     if (TsinceS1<0){
        if (TMinutes>60)  {PrevMins[0]=TMinutes-60;}
        TsinceS1=99;      // set led and display not in sync
     }
     if (TsinceS2<0){
        if (TMinutes>60)  {PrevMins[1]=TMinutes-60;}
        TsinceS2=99;
     }

     if (TsinceS1>60){led2.on();}else{led2.off();}           //if update longer than 60 minutes ago
     if (TsinceS2>60){led3.on();}else{led3.off();}           //if update longer than 60 minutes ago          

      Serial.println      ("TsinceS1:"+ String(TsinceS1) + " PrevMins[0]:" + String(PrevMins[0]));
      Serial.println      ("TsinceS2:"+ String(TsinceS2) + " PrevMins[1]:" + String(PrevMins[1]));

     #if  MIDNIGHT
     if (TsinceS1<0||TsinceS2<0){
      SensTim [0] = "00:00";
      SensTim [1] = "00:00";
      WriteToEeprom       ();
      Serial.println      ("Midnight restart");
      delay(500);
      esp_restart();      
     }
     #endif
     
     #if DEBUG
       Serial.print       ("sensor values:");
       Serial.print       (Humi[0]); 
       Serial.print       ("/");
       Serial.print       (Humi[1]);
       Serial.print       ("=");
       Serial.print       (gemhum);
       Serial.print       (" TsinceS1:");
       Serial.print       (TsinceS1);
       Serial.print       (" TsinceS2:");
       Serial.println     (TsinceS2);
       Serial.println     ("Eepsave #0:" + EepSave[0] + " #1:" + EepSave[1]);
     #endif
     
     Blynk.virtualWrite   (V0 ,SensTim[0]);
     Blynk.virtualWrite   (V1 ,SensTim[1]);
     Blynk.virtualWrite   (V4 ,Humi   [0]);
     Blynk.virtualWrite   (V5 ,Humi   [1]);
     Blynk.virtualWrite   (V6 ,SensMes[0]);
     Blynk.virtualWrite   (V7 ,SensMes[1]);
     Blynk.virtualWrite   (V10,Temp   [0]);
     Blynk.virtualWrite   (V11,Temp   [1]);
     Blynk.virtualWrite   (V12,String((SensMes[0].toInt()+Temp[0])/24)); 
     Blynk.virtualWrite   (V13,String((SensMes[1].toInt()+Temp[1])/24));  
     Blynk.virtualWrite   (V17,TsinceS1);
     Blynk.virtualWrite   (V18,TsinceS2);
}      
#endif 

#if USE_RELAIS
void   checkbuttons(){
  bool pressed = true;    // can differ from board 2 board
  if (digitalRead         (lbutton)==pressed){           // left
    Serial.print          ("left button pressed setting threshold to:");
    if(Threshold>0)       {Threshold--;}
    Serial.println        (Threshold);
    delay(250);
  }
  if (digitalRead         (rbutton)==pressed){           // right
    Serial.print          ("right button pressed setting threshold to:");   
    if(Threshold<99)      {Threshold++;}
    Serial.println        (Threshold);
    delay(250);
  }
  if (digitalRead         (BUT_IN)!=pressed){           // onboard button
    Serial.print          ("onboard button pressed "); 
  #if USE_U8G2
      if (LCDon){
        u8g2.clearBuffer  (); 
        u8g2.sendBuffer   ();
        LCDon=false;
      } else {
        LCDon=true;
      }
  #endif
    delay(500);
  }
}
void displayThresh(){
  #if USE_U8G2
   if (LCDon){
    u8g2.clearBuffer      ();
    u8g2.setFont          (u8g2_font_open_iconic_all_6x_t);//48 pix high
    u8g2.setCursor        (0,56);
    u8g2.print            (char(129));//setup 
    u8g2.setCursor        (56,48);
    u8g2.setFont          (u8g2_font_fub30_tf);//54 pix high
    u8g2.print            (Threshold); 
    u8g2.sendBuffer       ();
   }
  #endif
}
#endif 
