/* com 8(S2) 6(S1)
 * \Arduino\LORAWAN\TTGO-LoRa-Series-master\LoRaSender\LoRaSender-higrow-10.2
 * to use with
 * \Arduino\LORAWAN\TTGO-LoRa-Series-master\LoRaReceiver-12.73
 * for 
 * LilyGO TTGO T-Higrow ESP32 - DHT11 Sensor with LilyGO TTGO T-Higrow LoRa Shield - 868MHz
 * 
 * sends sensor reading as coded text to LORA T3 LoRa32 oled
 * then waits for confirmation message and goes to sleep (for TTGO HIGROW DHT11)
 */

#include <SPI.h>
#include <LoRa.h>
#include "board_def.h"        // settings of T-Higrow LoRa Shield 
#include "EEPROM.h"
int eeprom_read1;
int eeprom_read2;
int eeprom_read3;
int eeprom_read4;

bool measure  = false;        // pause to measure power usage
bool dhton    = false;        // measure temperature

unsigned int  count = 1;
String  ID    = "MYDATA";     // identifier
String  SN    = "01";         // !! sensor number SET THIS ACCORDING TP SENSOR!!
String  DN    = "1";          // destination number of receiver
String  TT    = "25.12";      // temp reading(5)
String  HH    = "99.09";      // hum reading (5)
String  MN    ;               // messagenumber

int    expLN  = 11;           // length of "MYDATA0?1OK"
String expID  = "MYDATA";     // coded "co41j1zqqea";
String normal = "ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890+-"; // use same strings as in receiver program
String coded  = "1234567890abcdefghijklmnopqrstuvwxyz-+";
String msgin;
String msgout;
String subst;
String recv;

// soil sensor
#define drydef 3450           // default value for dry sensor
#define wetdef 1645           // default value for wet sensor
int dry;
int wet;
int errors;

// temp sensor
#include "DHT.h";
#define DHTPIN 16
#define DHTTYPE DHT11

// Initialize DHT sensor for normal 16mhz Arduino
DHT dht(DHTPIN, DHTTYPE);
float hum;                    // Stores humidity value in percent
float temp;                   // Stores temperature value in Celcius

// from bme280HIGROWtest
#define SOIL_PIN                (32)
#define LED_PIN                 (16)
#define OB_BH1750_ADDRESS       (0x23)//lightmeter
#define BME_SCK 13
#define BME_MISO 12
#define BME_MOSI 11
#define BME_CS 10
#define TEMP_CORR 5
#define lo_button 35

// deepsleep settings
#define uS_TO_S_FACTOR 1000000  //Conversion factor for micro seconds to seconds
int sleepmins = 5;              //#minutes 2 sleep 70max;
int sleeptime = sleepmins * 60; // 1 mins * 60 seconds

void setup()
{
  Serial.begin        (115200); 
  while               (!Serial);
  Serial.println      (__FILE__);
  Serial.print        ("This sensor = ");
  Serial.println      (SN);

  pinMode             (lo_button, INPUT);
  int test=digitalRead(lo_button);
  Serial.print        ("Lower button ");
  if (test==1)        {Serial.print("not ");}  
  Serial.println      ("pressed!");
  EEPROM.begin        (32); // 4 integers = 3*4 = 16
  if (test==0){    // lower button pressed during startup
    wet   = wetdef;    
    dry   = drydef;
    count = 0;
    errors= 0;    
    EEPROM.writeInt(            0,count);
    EEPROM.writeInt(  sizeof(int),dry);
    EEPROM.writeInt(2*sizeof(int),wet);
    EEPROM.writeInt(3*sizeof(int),errors);
    EEPROM.commit  ();
    Serial.print   ("NEW ");
  } else {
    eeprom_read1 =     EEPROM.readInt  (0); 
    eeprom_read2 =     EEPROM.readInt  (sizeof(int)); 
    eeprom_read3 =     EEPROM.readInt  (sizeof(int)*2); 
    eeprom_read4 =     EEPROM.readInt  (sizeof(int)*3); 
    count=eeprom_read1;
    if (eeprom_read2>4000||eeprom_read2<2000){eeprom_read2=drydef;}
    dry=eeprom_read2;
    if (eeprom_read3>2000||eeprom_read3<1000){eeprom_read3=wetdef;}
    wet=eeprom_read3;
    errors=eeprom_read4;
  }
  Serial.print  ("EEprom values:");
  Serial.print  (count);
  Serial.print  (" dry:");
  Serial.print  (dry);
  Serial.print  (" wet:");
  Serial.print  (wet);
  Serial.print  (" errors:");
  Serial.println(errors);

//setup deepsleep  
  unsigned long     sleepingtime  = (sleeptime * uS_TO_S_FACTOR);
  esp_sleep_enable_timer_wakeup (sleepingtime);

//setup expected confirmation message    
  String myMessage = expID; //"MYDATA"
  myMessage+=SN;            // 01 or 02
  myMessage+=DN;            // destination
  myMessage+="OK";          // "MYDATA0?1OK"
  codit(myMessage,true);    // "MYDATA0?1OK" convert msgin to coded msgout
  expID=msgout;             // "co41j1zqqea"

if (measure){
  Serial.println("10 seconds to measure power");
  delay(10000);
  Serial.println("starting lorawan ...");
}
//setup lorawan
  SPI.begin(CONFIG_CLK, CONFIG_MISO, CONFIG_MOSI, CONFIG_NSS);
  LoRa.setPins(CONFIG_NSS, CONFIG_RST, CONFIG_DIO0);
  if (!LoRa.begin(BAND)) {
      Serial.println("Starting LoRa failed!");
      while (1);
  } else {Serial.println("Lora initialised!");}

if (measure){
  Serial.println("10 seconds to measure power");
  delay(10000);
  Serial.println("starting DHT ...");
}

  //tinycode 2 turn sensors on
  pinMode         (4, OUTPUT);
  digitalWrite    (4, HIGH);
  delay           (200);  
  Wire.begin      (25, 26);

  if (dhton){  // Start DHT Sensor
    dht.begin();  
    delay(2000); 
  }
}

void loop()
{
// Set values to send
// structure in a string IDIDIDSSDTTHHH     SS=source sender ID annex sensor number D=destination ID TT=temp HHH=humidity
// IDIDIDSSDTTTHHH00000       lenght 20
// String  ID  = "MYDATA";   // identifier         (6)
// String  SN  = "01";       // sensor number      (2)
// String  DN  = "1";        // destination number (1)
// String  TT  = "025";      // temp reading       (3) -99 to 999
// String  HH  = "100";      // hum reading        (3) 0 to 100
// String  MN   ="00000";    // messagenumber      (5)

// get soil reading
int humi      =   analogRead(SOIL_PIN);
if (humi>dry){    // auto set max value and write to eeprom
   dry=humi;
   EEPROM.writeInt(sizeof(int),dry);
   EEPROM.commit  ();
   Serial.print   ("New dry value written to eeprom:");
   Serial.println (dry);
   } 
if (humi<wet){    // auto set min value and write to eeprom
   wet=humi;
   EEPROM.writeInt(2*sizeof(int),wet);
   EEPROM.commit  ();  
   Serial.print   ("New wet value written to eeprom:");
   Serial.println (wet);
   }

Serial.print      ("analogRead humi:");
Serial.println    (humi);
HH    =       map (humi,wet,dry,100,0);
while (HH.length()<3){
   HH = "0" + HH;
}
Serial.print      ("value of   humi:");
Serial.println    (HH);

if (dhton){
  int T = dht.readTemperature();
  TT    = T;
  String addchar  ="0";             // pa. "002"
  if (T<0){TT="-"+TT;addchar=" ";}  // pa. " -2"
  while (TT.length()<3){
      TT = addchar + TT;
  }  
} else { 
    TT="023";             // dummy temp value
}
Serial.print      ("value of   temp:");
Serial.println    (TT);
  
MN    = String    (count);
while (MN.length()<=4){
  MN = "0" + MN;
}
count++;
if (count>50000){count=0;}

  // send message and wait 4 confirmation max 3 times
for (int j = 0; j <3; j++) { 

  //debug nr errors(=retries) temporarely use temp space to store error value because serial mon not working
  TT = String(errors);
  while (TT.length()<3){TT = "0" + TT;}
  //
  String myMessage=ID+SN+DN+TT+HH+MN;  
  codit (myMessage,true);  // convert msgin to coded msgout
  myMessage = msgout;

  int lenIN=0;
  Serial.print    ("Sending  :");
  Serial.print    (myMessage);
  Serial.print    (" len=");
  Serial.println  (myMessage.length());   
  LoRa.beginPacket();
  LoRa.print      (myMessage);
  LoRa.endPacket  ();
  LoRa.receive    ();
  // wait 4 conformation message max 3x "MYDATA0?1OK"
  delay (100);//tune this and compare time on receiver com port monitor
            
  for (int i = 0; i <3; i++) {
     Serial.print      ("trying to receive confirmation message #");         
     Serial.println    (i+1);         
     if (LoRa.parsePacket()) {
       recv = "";
       while (LoRa.available()){recv += (char)LoRa.read();}
     }
     lenIN     =          recv.length();        // length of "MYDATA011OK"
     if (lenIN!=0){
        Serial.print      ("received :");      
        Serial.print      (recv); 
        Serial.print      (" ");
        Serial.print      (recv.length());
        Serial.println    (" long");
        codit(recv,false);// convert coded msgin to msgout
        if (recv==expID)  {
          Serial.println  ("Received OK! Going 2 sleep mmmmm...");       
          EEPROM.writeInt (0,count); 
          EEPROM.writeInt (3*sizeof(int),errors);
          EEPROM.commit   (); 
          Serial.print    ("Counter value written to eeprom:");
          Serial.println  (count);
          digitalWrite    (SOIL_PIN, LOW);  
          digitalWrite    (4, LOW); //turn off GPIO16 
          Serial.println  ("Going to sleep for  " + String(sleeptime/60) + " Minutes");
          delay           (200);
          esp_deep_sleep_start();  
          break;          // never reached           
        }
     } 
     Serial.print          ("expected package long ");
     Serial.print          (expLN);
     Serial.print          (": ");
     Serial.println        (expID); 
     delay (1000); 
    }
  }   

  errors++;          
  Serial.println        ("Going to sleep unsatisfied for " + String(sleeptime/60) + " Minutes");
  EEPROM.writeInt       (0,count); 
  EEPROM.writeInt       (3*sizeof(int),errors);
  EEPROM.commit         (); 
  delay                 (200);
  esp_deep_sleep_start  ();  
}

void codit(String secretmessage, bool mydir){
  msgout="";
  Serial.print      ("secret   :");
  Serial.println    (secretmessage);
  for (int i = 0; i < secretmessage.length(); i++) {
      String x      = secretmessage.substring(i,i+1);
      if (mydir){
        int y = normal.indexOf   (x);
        if (y>normal.length()){subst="*";}
        else {subst = coded.substring   (y,y+1);}            
      } else {
        int y = coded.indexOf    (x);
        if (y>coded.length()){subst="*";}
        else {subst = normal.substring (y,y+1);}
      }
      msgout += subst;
  }
  Serial.print      ("converted:");
  Serial.println    (msgout);
  if (secretmessage.length()!=msgout.length()){Serial.println("Error: character(s) not found check String normal!");}
}
