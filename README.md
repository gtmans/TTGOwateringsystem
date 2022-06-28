##### THIS IS WORK IN PROGRESS - NOT FINISHED YET #####

  The goal of this project is to create a wireless sensoring system that controls my watering system (a rainbird raincomputer)
  Whenever one of the soilsensors signals the ground is to dry it triggers a relay that gives input to the raincomputer  

![ttgo-water-DHT11](https://user-images.githubusercontent.com/96861311/150639059-e4c2bfb7-3b94-4467-84b0-293fc4706556.jpg)

  But first I built a testing system(weatherstation). I started with these 3 modules of prox 12 euro each
  - LilyGO TTGO T-Display V1.1 ESP32 - with 1.14 inch TFT Display
  - LilyGO TTGO T-Higrow ESP32 - BME280 Sensor
  - LilyGO TTGO T-Higrow ESP32 - DHT11 Sensor (its easier to buy 2 of the same Higrows but I got stuck with these)

  I created 2 HTTP servers and one HTTP client inspired by  
  https://RandomNerdTutorials.com/esp32-client-server-wi-fi/
  https://github.com/Xinyuan-LilyGO/TTGO-LoRa-Series 
  https://gist.github.com/jenschr/0fc981415233e0751f22972811b4957f

  Wrote these 2 server programs:
  Program myESP32_server_Higrow_DHT11  Sensorserver1 IP 192.168.2.10 sleeps 70 awake 2 mins white casing
  Program myESP32_server_Higrow_BME280 Sensorserver2 IP 192.168.2.11 sleeps 70 awake 2 mins red casing
  The sensors will be placed outdoors and will be battery based so to make them power efficient 
  I let hem sleep as long as possible (the most I could realise was 70 minutes). So it wakes up every 70 minutes
  and is a server for 2 minutes then goes 2 sleep A 3D printed case can be foud on Thingiverse or Github
  
  Wrote a test program: myESP32_client_T-Display
  
  Next step was to create the base (client) station with humidity threshold setup and a relais
    
  ![ttgo-water-0 9display](https://user-images.githubusercontent.com/96861311/150639033-a90b288c-eefd-4a6d-8f29-627297b8e9ea.jpg)
    
  2022-01 
  Built a client based on LilyGO TTGO ESP8266 with 0.91 inch OLED Display
  Started testing:
    
  test1: 2 servers 2 min online every 70 minutes with 800Ma  lipo lasting 21 days!   
  test2: 2 servers 1 min online every 70 minutes with 800Ma  lipo lasting 32 days (2022-20-01 till 2022-21-02)
  test3: 1 servers 1 min online every 70 minutes with 1500Ma lipo lasting 40 days (2022-26-02 till 2022-06-04)
     
  2022-03 new perspective: ESPNOW
  client: LilyGO TTGO ESP8266 - with 0.91 inch OLED Display
  server: LilyGO TTGO T-Higrow ESP32 - DHT11 Sensor 
  Read about ESPNOW and did some tests. Started rewriting the code by using ESPNOW and added a lowpower client using D1 mini & capacitive soil sensor v2.0
  The lipo powered sensors in the garden will wake-up ever 70 minutes and send a message to the client and then check if message is received and goes to deepsleep.   
  If not it will wait 5 seconds and retry 3 times before going to deepsleep. With 800Ma lipo lasting prox 50 days. Sample files in ESPNOW directory
    
  2022-4 D1 mini V3.1 with capacitive soilsensor v2.0 getting 5v from LOLIN batteryshield v1.3 (every 70 minuten in deepsleep) with 800ma lipo is no success: lasts   
  only about 4 days! Going on with ESPNOW and reading things about LORA ...  
   
 ![lora-receiver](https://github.com/gtmans/TTGOwateringsystem/blob/main/pics/lora-receiver.jpg)
 
  2022-05 Started Lorawan server and client. Is working but the client stops working after some time .... 
  client: LilyGO TTGO T3 LoRa32 868MHz V2.1.6 ESP32
  server: LilyGO TTGO T-Higrow ESP32 - DHT11 Sensor with LilyGO TTGO T-Higrow LoRa Shield - 868MHz
    

    
the story continues here:
   more in the ![readme.md](https://github.com/gtmans/TTGOwateringsystem/blob/main/lorawan/README.md) in the lorawan directory: 


![lora-transmitter](https://github.com/gtmans/TTGOwateringsystem/blob/main/pics/lora-transmitter.jpg)

