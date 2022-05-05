/* LoRaReceiver-higrow to use with LoRaSender-higrow
 * for (ESP32 Dev Module) LilyGO TTGO T3 LoRa32 868MHz V2.1.6 ESP32
 * reads text records from lorasenders higrow
 * Mans Rademaker 2022
 */
#include <SPI.h>
#include <LoRa.h>
#include "lora_board_def.h"
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include "gewoon_secrets.h" //or use 2 lines below
//#define WIFI_SSID       "yourWIFISSID"     // ssid
//#define WIFI_PASSWORD   "yourWIFIPassword" // password

// write/read eeprom after reset if not wanted use //
#include "EEPROM.h"
bool eeprom       = true;        // SET! store last read values in eeprom
bool espres       = false;       // SET! restart ESP every 60 min to avoid hanging(set value interval below)
bool debug        = false;       // SET! dump incoming string to screen or just their values
bool sndextramsg  = true;        // SET! send 2 or 1 confirmation message
bool doreply      = true;        // SET! send confirmation message or not
bool usewifi      = true;        // SET! use wifi 2 get timestamp
long interval     = 3600000;     // SET! interval 60 minutes to restart ESP every 60 min to avoid hanging

#define NrSensors 2              // SET!
String Sensors [NrSensors];
String SensMes [NrSensors]={"?????","?????"};
String SensTim [NrSensors]={"waiting","waiting"};
String EepSave [NrSensors];
int    NrMsgs  [NrSensors]={0,0};
int    count  = 0;               // total nr of correct received packages
int    expLEN = 20;              // SET! length of "MYDATA01102500100000"
String ID  = "MYDATA";           // SET! your chosen identifier
String expID  = ID;         
// choose your own coding/decoding strings but use same strings as in sender program
String normal = "ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890+-";//decode
String coded  = "1234567890abcdefghijklmnopqrstuvwxyz-+";//code (pa A becomes 1)
String msgin;
String msgout;
String subst;
String recv;

String myMessage;
String sentence;
String SN;                        // sensor number
String DN;                        // destination number
String TT;                        // temp reading(3)
String HH;                        // hum reading (3)
String MN;                        // messagenumber
int    T;                         // temp   reading  as integer
int    H;                         // hum    reading  as integer
int    S;                         // sensor  number as integer
int    M;                         // message number as integer
long   previousMillis = 0;      

//RTC
StaticJsonDocument<1000> doc;
WiFiUDP   ntpUDP;                // Define NTP Client to get time
NTPClient timeClient(ntpUDP);    // Variables to save date and time
String    formattedDate;
String    TimeStamp;

//OLED
OLED_CLASS_OBJ display(OLED_ADDRESS, OLED_SDA, OLED_SCL);
        
void setup()
{
    Serial.begin(115200);
    while (!Serial);
    delay (1000);
    Serial.println    (__FILE__);
    if (OLED_RST > 0) {
        pinMode       (OLED_RST, OUTPUT);
        digitalWrite  (OLED_RST, HIGH);
        delay(100);
        digitalWrite  (OLED_RST, LOW);
        delay(100);
        digitalWrite  (OLED_RST, HIGH);
    }
    display.init              ();
    display.flipScreenVertically();
    display.clear             ();
    display.setFont           (ArialMT_Plain_16);
    display.setTextAlignment  (TEXT_ALIGN_CENTER);
    display.drawString        (display.getWidth() / 2, display.getHeight() / 2, LORA_SENDER ? "LoRa Sender" : "LoRa Receiver");
    display.display();

if (usewifi){
    WiFi.begin(ssid,password);
    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
        display.clear         ();
        Serial.println        ("WiFi Connect Fail");
        display.drawString    (display.getWidth() / 2, display.getHeight() / 2, "WiFi Connect Fail");
        display.display       ();
        delay(2000);
        esp_restart();
    }
    Serial.print              ("Connected : ");
    Serial.println            (WiFi.SSID());
    Serial.print              ("IP:");
    Serial.println            (WiFi.localIP().toString());
    display.clear             ();
    display.drawString(display.getWidth() / 2, display.getHeight() / 2, "IP:" + WiFi.localIP().toString());
    display.display           ();
}
    SPI.begin                 (CONFIG_CLK, CONFIG_MISO, CONFIG_MOSI, CONFIG_NSS);
    LoRa.setPins              (CONFIG_NSS, CONFIG_RST, CONFIG_DIO0);

    if (!LoRa.begin(BAND)){
        Serial.println        ("Starting LoRa failed!");
        while (1);
    }
    if (!LORA_SENDER) {
        Serial.println        ("LoRa initialised!");
        display.clear         ();
        display.drawString    (display.getWidth() / 2, display.getHeight() / 2, "LoraRecv Ready");
        display.display       ();
    }
    display.setTextAlignment  (TEXT_ALIGN_LEFT);
    if (usewifi){
        //RTC Initialize a NTPClient to get time
        timeClient.begin          (); 
        timeClient.setTimeOffset  (7200);  // NOT DST 3600
        while                     (!timeClient.update()) {timeClient.forceUpdate();}
    }
    ReadFromEeprom            ();
    previousMillis = millis   ();
    LoRa.receive              ();
}

void loop(){
  unsigned long currentMillis = millis(); 
  if(currentMillis - previousMillis > interval) { 
    if (espres){
      Serial.println          ("restarting ESP");
      WriteToEeprom           ();
      delay                   (1000);      
      ESP.restart             ();
    }
    Serial.println            ("resetting looptimer");
    previousMillis =          currentMillis;
  }
  
  if (LoRa.parsePacket()) {
      recv = "";
      while (LoRa.available()) {recv += (char)LoRa.read();}
      int lenIN   =       recv.length();      // length of "MYDATA01100200100047"
      Serial.print        ("received package long ");      
      Serial.println      (lenIN); 
      if                  (lenIN==expLEN){

        //XTRA CHK 2 PREVENT HANGING ESP
        String CHK = recv.substring(0,1); //"c"
        int p = coded.indexOf     (CHK);  //12
        subst = normal.substring  (p,p+1);//"M"
        Serial.print      (CHK);   
        Serial.print      (p);   
        Serial.println    (subst);       
        if (expID.substring(0,1)==subst){ // extra check: 1st character of message is as expected
          codit      (recv,false);        // now convert coded msgin to msgout
          recv    =   msgout;
          ID      =   recv.substring(0,6);// "MYDATA"          
          count++;     
        } else { ID="X";}
        //END XTRA CHK        
          
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

          if (doreply){
            Serial.print        ("sending  :");   
            Serial.println      (myMessage);  
            LoRa.beginPacket    ();
            LoRa.print          (myMessage);
            LoRa.endPacket      ();
          } else {Serial.println("Not sending confirmation message.");  }      


          if (usewifi){
            formattedDate   = timeClient.getFormattedDate();
            int pos         = formattedDate.indexOf("T");
            TimeStamp       = formattedDate.substring(pos+1, formattedDate.length()-1-3);
          } else              {TimeStamp="12:34";}
          S = SN.toInt();      
          ParseReadings(S,true);          // store data in arrays and update counter 
                     
          if (debug){displayDebug();} else {displayOled();}

          if(sndextramsg&&doreply){       //SEND AGAIN 2 BE SURE        
            delay             (1000); 
            Serial.print      ("sending  :");   
            Serial.println    (myMessage);  
            LoRa.beginPacket  ();
            LoRa.print        (myMessage);
            LoRa.endPacket    ();
          }
          WriteToEeprom       ();         // update eeprom values for reset or restart
          if (doreply){LoRa.receive();}   // after sending start receiving again          
      } // expected ID 
      else { Serial.println("Package is not mine ... wrong ID ignoring package")    ; } // end of some packet same length
    } // end of len = expexted len
    else   { Serial.println("Package is not mine ... wrong length ignoring package"); } // end of some packet
  }   // end of parsepacket
}     // end of loop

void codit(String secretmessage, bool mydir){ // mydir true => encrypt false => decrypt message
  msgout="";
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
  Serial.print      ("converted:");
  Serial.println    (msgout);
  if (secretmessage.length()!=msgout.length()){
    Serial.println  ("Error: output string wrong length!");
    msgout =        secretmessage;
  }
}

void WriteToEeprom (){
    if (!eeprom){return;}
    EEPROM.begin      (64); 
    //11:11MYDATA0111350020002012:12MYDATA02113500200020" sample format of message
    sentence        = SensTim[0]+EepSave[0]+SensTim[1]+EepSave[1];
    if (sentence.length()>64){ // error in message do not overwrite eeprom 2 be sure not 2 hangup esp
        sentence="12:129999999999999999999912:1299999999999999999999";
    }
    EEPROM.writeString(0, sentence);
    Serial.print      ("Written to eeprom:");
    Serial.print      (sentence);
    Serial.print      (" long:");
    Serial.println    (sentence.length());
    EEPROM.commit     ();
}
void ReadFromEeprom   (){
    if (!eeprom){return;}
    EEPROM.begin      (64); 
    sentence = EEPROM.readString (0);
    Serial.print      ("Read from eeprom:");
    Serial.print      (sentence);
    Serial.print      (" long:");
    Serial.println    (sentence.length());
    //11:11MYDATA0111350020002012:12MYDATA02113500200020"
    SensTim [0] =     sentence.substring(0    ,5      );// "11:11"
    SensTim [1] =     sentence.substring(0+25 ,0+25+5 );// "12:12"
    EepSave [0] =     sentence.substring(5    ,5+20   );// "MYDATA01113500200020"
    EepSave [1] =     sentence.substring(5+25 ,5+25+20);// "MYDATA02113500200020"
    Serial.println    ("Interpreting latest packages read from eeprom ...");       
    breakdown         (EepSave[0]);
    ParseReadings     (1,false);
    breakdown         (EepSave[1]);
    ParseReadings     (2,false);
    displayOled       ();
}

void breakdown(String input){
  ID  = input.substring(0 , 6);//"MYDATA"
  SN  = input.substring(6 , 8);//"01";     sensor number
  DN  = input.substring(8 , 9);//"1";      destination number
  TT  = input.substring(9 ,12);//"025";    temp reading(3)
  HH  = input.substring(12,15);//"099";    hum reading (3)
  MN  = input.substring(15,20);//"01234";  messagenumber
  T   = TT.toInt();            //25 
  TT  = String(T);
  H   = HH.toInt();            //99
  HH  = String(H);
  S   = SN.toInt();            //1       
  M   = MN.toInt();            //1234
  Serial.print      ("total received packages:");
  Serial.print      (count);
  Serial.print      (" this one is ");
  Serial.print      (M);
  Serial.print      (" from sensor ");
  Serial.print      (S);
  Serial.print      (" message->");
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
}

void ParseReadings(int tsensor,bool updmsg){// sensor nr 1 or 2 stored in aray[0] or array[1]
     Sensors[tsensor-1]  = "S"+SN+" "+HH+"%"+" "+TT+" errors";
     SensMes[tsensor-1]  = MN;
     if (updmsg){
      SensTim[tsensor-1] = TimeStamp;
      EepSave[tsensor-1] = recv;
      NrMsgs [tsensor-1]++;
     }
}

void displayDebug(){                    // just dump received string on display
     display.clear();
     String  PART1  = recv.substring(0,10);
     String  PART2  = recv.substring(10,20);
     display.drawString  (0 ,0, "Incoming data");
     display.drawString  (0 ,16, PART1);
     display.drawString  (0 ,32, PART2);
     display.drawString  (0 ,48, "from S");
     display.drawString  (48,48, SN);
     display.display();
}

void displaySecret(String in, String out){
     display.clear();
     display.drawString  (0,16,in);
     display.drawString  (0,32,out);
     display.display     ();
}

void displayOled(){
     display.clear();
     display.drawString  (0    ,0, SensTim [0]);               // timestamp of latest message
     display.drawString  (64   ,0, SensTim [1]);
     display.drawString  (0    ,16,Sensors [0]);               // formatted string with data sensor1
     display.drawString  (0    ,32,Sensors [1]);
     display.drawString  (0 +24,48,SensMes [0].substring(2,5));// nr of sent     messages according to sender(sensor)
     display.drawString  (64+24,48,SensMes [1].substring(2,5));             
     display.drawString  (0    ,48,String(NrMsgs[0]));         // nr of received messages according to receiver
     display.drawString  (64   ,48,String(NrMsgs[1]));             
     display.display     ();
}
