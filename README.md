# TTGOwateringsystem

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
