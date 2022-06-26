### PLEASE Be aware that this is an ongoing project! ####

The goal of this project is to create a wireless sensoring system that controls my watering system (a rainbird raincomputer) 
Whenever one of the soilsensors signals the ground is to dry it triggers a relay that gives input to the raincomputer

April 2022 I discovered TTGO Lorawan modules which look promising for powerconsumption and range compared to WIFI-ESPNOW.
Inspiration for this project was Timm Bogner's FDRS system:https://github.com/timmbogner/Farm-Data-Relay-System

To test and understand how it works I built a sort of LORANOW construction myself: 
The sensor modules send their readings to the client module which send a confirmation message to the sender. 
If confirmed the sender goes to deepsleep for 70 minutes (max. reliable time) In this test program time is 5 minutes.

![lora-transmitter](https://github.com/gtmans/TTGOwateringsystem/blob/main/pics/lora-transmitter.jpg)
   
The LoRaReceiver-Higrow.ino is for for (ESP32 Dev Module):LilyGO TTGO T3 LoRa32 868MHz V2.1.6 ESP32 and 
the LoRaSender-Higrow.ino   is for LilyGO TTGO T-Higrow ESP32 - DHT11 Sensor with LilyGO TTGO T-Higrow LoRa Shield - 868MHz
First power measurements of this sender setup is sleep 0,02mA/awake 40mA/lora activity 40-55mA where WIFI uses prox. 125 mA
Both are TEST programs. Finally the receiver will be added a relais and possibly a webserver. 

LORANOW construction.
All information is packet in a text string that is sent using LoRa.beginPacket();LoRa.print();LoRa.endPacket();
Because the text is readable it is simply coded:every character is replaced by another character. 
After sending the sender waits for a confirmation message by using LoRa.receive();LoRa.parsePacket();LoRa.available() and LoRa.read();

2022 may 5 status:
This setup is working perfectly with 2 sensors and 1 receiver exept for the fact that the receiver unit stops working after a few hours (12-36 sender messages)...
No clue at all where it stops - unit seems to hang but display stays on - serial monitor stops after some time (windows)

2022-05-09 Seen something interesting because the serial monitor kept on running longer as usual: The main loop keeps running while there are no more messages received (LoRa.parsePacket() does not get true). Every 60 mins it still does "resetting looptimer" as programmed. So it does not hang but stops receiving ....

2022 06-06 Problems solved. My problems were caused by: 
- testing all modules within centimeters apart from eachother
- with horizontal antennas
- with max radiopower
I made several adjustments and the test programs LoRaReceiver-higrow-16 and LoRaSender-higrow-10.7 (in workingdir) work OK and are in test
I integrated Blynk dashboard (where you can set the threshold and check all important parameters):

![blynk_dash](https://github.com/gtmans/TTGOwateringsystem/blob/main/pics/Blynk-dashboard%202022-06-06%20162047.png)

2022-06-13 1st calculation of endurance is 179 days on a 800 mA lipo with sleeptime of 60 minutes (based on a test with 5 minutes sleepingtime)

2022-06-26 System up and running! (pics)
![LoRa1.0U8G2case](https://github.com/gtmans/TTGOwateringsystem/blob/main/pics/LoRa1.0U8G2case.png)
![gardensensor](https://github.com/gtmans/TTGOwateringsystem/blob/main/pics/lorasender-case-garden.png)
