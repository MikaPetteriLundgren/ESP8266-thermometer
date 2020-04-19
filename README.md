ESP-8266 thermometer
=================

ESP-8266 based thermometer implementation for Domoticz home automation system.
The sketch includes the following features:

* Measures temperature from DS18B20 temperature sensor 
* Creates JSON formatted message from the measured temperature values
* Sends the JSON formatted message to the Domoticz via MQTT

ESP-8266 is connected to local network with Wifi.
Domoticz/MQTT broker is listening "domoticz/in" MQTT topic for the incoming messages.
Memory allocation for outgoing MQTT messages is calculated using [ArduinoJson Assistant](https://arduinojson.org/v6/assistant).

The sketch reads temperature values from DS18B20 temperature sensor via OneWire bus using Dallas Temperature Control Library.
The read temperature value is sent to Domoticz via MQTT protocol in JSON format.
MQTT callback functionality is not used in the sketch

The sketch needs ESP8266-thermometer.h header file in order to work. The header file includes settings for the sketch.

If the MQTT connection has been disconnected, reconnection will take place. If the reconnection fails 10 times, Wifi connection is disconnected and initialized again.

It's possible to print amount of free RAM memory via serial port by uncommenting `#define RAM_DEBUG` line
It's possible to print more debug information via serial port by uncommenting `#define DEBUG` line

The sketch will need following HW and SW libraries to work:

**HW**

* ESP-8266 based MCU (Lolin D1 mini used in this implementation)
* DS18B20 temperature sensor

**Libraries**

* PubSubClient for MQTT communication
* ArduinoJson for JSON message serializing
* OneWire.h for OneWire communication
* DallasTemperature for DS18B20 controlling
* Time and TimeAlarms for setting up timers

**HW connections**

The Lolin D1 mini is connected to the DS18B20 temperature sensor using parasite mode. 
This repository includes schematics of the system.
