Program myESP32_client_T-Display (this program) Just 2 test things I created a sort of monster with these functions:

displays several data gained from internet such as time, weather, and the 2 servers mentioned above

displays screennumber (right above)

displays the status of the 2 sensors ** on/ofline sensr1&1

displays the health of the 2 sensors XX seen within 70 minutes

screen 1 displays internet time

screen 2 displays internet weather (temp, min temp, max temp, humidity, wind speed and direction) and minutes since last reading

screen 3 displays reading of the first sensor DHT11 (temp, humidity, soil humidity) and minutes since last reading

screen 4 displays reading of the first sensor BME280 (temp, humidity, pressure, soil humidity) and minutes since last reading

automated screen roulation

all screens have a landscape mode activated by long press right button

switch screens by short press right button

set clarity by short press left button

check server directly by long press left button

a continous measuring mode

a sleep mode option to save battery. Set go2sleep = true and sleeptime to nr. of milliseconds. After that time readings are stored to eeprom and read from eeprom when starting up
