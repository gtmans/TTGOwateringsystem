  The goal of this project is to create a wireless sensoring system that controls my watering system (a rainbird raincomputer)
  Whenever one of the soilsensors signals the ground is to dry it triggers a relay that gives input to the raincomputer  

  But first I built a testing system(weatherstation). I started with these 3 modules of prox 12 euro each
  - LilyGO TTGO T-Display V1.1 ESP32 - met 1.14 inch TFT Display
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
  
  Program myESP32_client_T-Display (this program)
  Just 2 test things I created a sort of monster with these functions:
  - displays several data gained from internet such as time, weather, and the 2 servers mentioned above
  - displays screennumber (right above)
  - displays the status of the 2 sensors ** on/ofline sensr1&1
  - displays the health of the 2 sensors XX seen within 70 minutes
  - screen 1 displays internet time
  - screen 2 displays internet weather (temp, min temp, max temp, humidity, wind speed and direction) and minutes since last reading  
  - screen 3 displays reading of the first sensor DHT11  (temp, humidity, soil humidity) and minutes since last reading
  - screen 4 displays reading of the first sensor BME280 (temp, humidity, pressure, soil humidity) and minutes since last reading
  - automated screen roulation
  - all screens have a landscape mode activated by long press right button
  - switch screens by short press right button
  - set clarity by short press left button
  - check server directly by long press left button
  - a continous measuring mode
  - a sleep mode option to save battery. Set go2sleep = true and sleeptime to nr. of milliseconds. 
    After that time readings are stored to eeprom and read from eeprom when starting up
