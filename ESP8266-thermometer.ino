
/*
   Sketch is implemented for Lolin D1 Mini powered by ESP-8266EX MCU.

   The sketch reads temperature values from DS18B20 temperature sensor via OneWire bus using Dallas Temperature Control Library
   The read temperature value is sent to Domoticz via MQTT protocol
   MQTT callback functionality is not used in the sketch

   The sketch needs ESP8266-thermometer.h header file in order to work. The header file includes settings for the sketch.
*/

#include "/Users/Miksu/Documents/Arduino/Projektit/ESP8266-thermometer/ESP8266-thermometer.h" // Header file containing settings for the sketch
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Time.h>
#include <TimeAlarms.h>

// Wifi network settings
char ssid[] = NETWORK_SSID; // Network SSID (name) is defined in the header file
char pass[] = NETWORK_PASSWORD; // Network password is defined in the header file
int status = WL_IDLE_STATUS; // Wifi status
IPAddress ip; // IP address
WiFiClient wifiClient; // Initialize Wifi Client
byte mac[6]; // MAC address of Wifi module

// OneWire and Dallas temperature sensors library are initialized
#define ONE_WIRE_BUS 2 // OneWire data wire is plugged into GPIO 2 of the ESP8266. Parasite powering scheme is used.
OneWire oneWire(ONE_WIRE_BUS); // Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
DallasTemperature sensors(&oneWire); // Pass our oneWire reference to Dallas Temperature.

// Temperature measurement variables are initialized
const unsigned int tempMeasInterval = 3600; // Set temperature measurement interval in seconds. Default value 3600s (1h)
float Temperature = 0; // Measured temperature is stored to this variable
DeviceAddress thermometerAddress; // Array for DS18B20 device address

// MQTT callback function header
void mqttCallback(char* topic, byte* payload, unsigned int length);

//MQTT initialization
PubSubClient mqttClient(MQTT_SERVER, 1883, mqttCallback, wifiClient); // MQTT_SERVER constructor parameter is defined in the header file
char clientID[50];
//char topic[50];
char msg[80];
char topic[] = MQTT_TOPIC; // Topic for outgoing MQTT messages to the Domoticz. The MQTT_TOPIC is defined in the header file
char subscribeTopic[ ] = MQTT_SUBSCRIBE_TOPIC; // This topic will be listened for incoming MQTT messages from the Domoticz. The MQTT_SUBSCRIBE_TOPIC is defined in the header file
int mqttConnectionFails = 0; // If MQTT connection is disconnected for some reason, this variable is increment by 1

//MQTT variables
const int temperatureSensorIDX = 1481; // IDX of the temperature sensor in Domoticz

//#define DEBUG // Comment this line out if there is no need to print debug information via serial port
//#define RAM_DEBUG // Comment this line out if there is no need to print RAM debug information via serial port


void setup()
{
  Serial.begin(115200); // Start serial port
  //while (!Serial) ; // Needed with Arduino Leonardo only

  // Start Wifi
  startWiFi();
  WiFi.macAddress(mac);

  //Print MAC address
  Serial.print(F("MAC address: "));
  for (int i = (sizeof(mac)-1); i>=0; i--)
  {
    Serial.print(mac[i],HEX);
    Serial.print(":");
  }
  Serial.println();

  // MQTT client ID is constructed from the MAC address in order to be unique. The unique MQTT client ID is needed in order to avoid issues with Mosquitto broker
  String clientIDStr = "Lolin_D1_Mini_";

  for (int i = (sizeof(mac)-1); i>=0; i--)
  {
    clientIDStr.concat(mac[i]);
  }

  Serial.print(F("MQTT client ID: "));
  Serial.println(clientIDStr);
  clientIDStr.toCharArray(clientID, clientIDStr.length()+1);

  //MQTT connection is established and topic subscribed for the callback function
  mqttClient.connect(clientID) ? Serial.println(F("MQTT client connected")) : Serial.println(F("MQTT client connection failed...")); //condition ? valueIfTrue : valueIfFalse
  mqttClient.subscribe(subscribeTopic) ? Serial.println(F("MQTT topic subscribed succesfully")) : Serial.println(F("MQTT topic subscription failed...")); //condition ? valueIfTrue : valueIfFalse

  // Timers are initialised
  Alarm.timerRepeat(tempMeasInterval, tempFunction);

  // Start the DallasTemperature library and perform the first measurement and send it to Domoticz
  sensors.begin();

  if (sensors.getDeviceCount() > 0 && sensors.getAddress(thermometerAddress, 0))
  {
    Serial.println(F("DS18B20 sensor found"));
    Serial.print(F("Sensor address: "));
    printAddress(thermometerAddress);
    Serial.println();
    Serial.print(F("Parasite power mode: "));
    sensors.isParasitePowerMode() ? Serial.println(F("On")) : Serial.println(F("Off")); //condition ? valueIfTrue : valueIfFalse
    Serial.print("Sensor resolution: ");
    Serial.print(sensors.getResolution(thermometerAddress), DEC); 
    Serial.println(F("bits"));
    tempFunction();
  }
  else
  {
    Serial.println(F("No DS18B20 sensor found...."));
  }

  Serial.println(F("Setup completed succesfully!\n"));
}


void loop()
{
    //If connection to MQTT broker is disconnected. Connect and subscribe again
    if (!mqttClient.connected())
    {
      printClientState(mqttClient.state()); // print the state of the client
      
      (mqttClient.connect(clientID)) ? Serial.println(F("MQTT client connected")) : Serial.println(F("MQTT client connection failed...")); //condition ? valueIfTrue : valueIfFalse

      //MQTT topic subscribed for the callback function
      (mqttClient.subscribe(subscribeTopic)) ? Serial.println(F("MQTT topic subscribed succesfully")) : Serial.println(F("MQTT topic subscription failed...")); //condition ? valueIfTrue : valueIfFalse

      mqttConnectionFails +=1; // If MQTT connection is disconnected for some reason this variable is increment by 1

      // Wifi connection to be disconnected and initialized again if MQTT connection has been disconnected 10 times
      if (mqttConnectionFails >= 10)
      {
        Serial.println(F("Wifi connection to be disconnected and initialized again!"));
        WiFi.disconnect();
        mqttConnectionFails = 0;
        startWiFi();
      }
    }

    mqttClient.loop(); //This should be called regularly to allow the MQTT client to process incoming messages and maintain its connection to the server.
    Alarm.delay(0); //Timers are only checks and their functions called when you use this delay function. You can pass 0 for minimal delay.
    delay(1000);

   #if defined RAM_DEBUG
     Serial.print(F("Amount of free RAM memory: "));
     Serial.print(ESP.getFreeHeap()/1024); // Returned value is in Bytes which is converted to kB
     Serial.println(F("kB")); //ESP-8266EX has 2kB of RAM memory
   #endif
}

void startWiFi() //ESP8266 Wifi
{
  WiFi.begin(ssid, pass);

  Serial.print(F("Connecting to Wi-Fi network "));
  Serial.println(ssid);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.println(F("Connected to WiFi"));
  Serial.print(F("IP address: "));
  Serial.println(WiFi.localIP());
}

String createMQTTPayload(int idx) //Create MQTT message payload. Returns created payload as a String.
{
  char buffer[10]; // Needed with dtostrf function
  String temperatureString = dtostrf(Temperature, 5, 1, buffer); //Converts float temperature to String with 1 digit precision

  StaticJsonDocument<104> doc; // Capacity has been calculated in arduinojson.org/v6/assistant

  // Add values in the JSON document
  doc["command"] = "udevice";
  doc["idx"] = 1481;
  doc["nvalue"] = 0;
  doc["svalue"] = temperatureString;

  // Generate the prettified JSON and store it to the string
  String dataMsg = "";
  serializeJsonPretty(doc, dataMsg);

	return dataMsg;
}

void sendMQTTPayload(String payload) // Sends MQTT payload to the MQTT broker / Domoticz
{

	// Convert payload to char array
	payload.toCharArray(msg, payload.length()+1);

    //If connection to MQTT broker is disconnected, connect again
  if (!mqttClient.connected())
  {
    (mqttClient.connect(clientID)) ? Serial.println(F("MQTT client connected")) : Serial.println(F("MQTT client connection failed...")); //condition ? valueIfTrue : valueIfFalse - This is a Ternary operator
  }

	//Publish payload to MQTT broker
	if (mqttClient.publish(topic, msg))
	{
		Serial.println(F("Following data published to MQTT broker: "));
		Serial.print(F("Topic: "));
    Serial.println(topic);
		Serial.println(payload);
		Serial.println();
	}
	else
  {
		Serial.println(F("Publishing to MQTT broker failed"));
  }
}

// mqttCallback function handles messages received from subscribed MQTT topic(s)
void mqttCallback(char* topic, byte* payload, unsigned int length)
{
  // MQTT callback functionality is not used in the sketch

  #if defined DEBUG
  Serial.print(F("MQTT message arrived ["));
  Serial.print(topic);
  Serial.println("] ");

  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }

  Serial.println();
  Serial.print(F("Size of the payload is: "));
  Serial.print(length);
  Serial.println();
  #endif
}

void printClientState(int clientState) // Prints MQTT client state in human readable format. The states can be found from here: https://pubsubclient.knolleary.net/api.html#state
{

  Serial.print(F("State of the MQTT client: "));
  Serial.print(clientState);
  
  switch (clientState)
  {
   case -4:
     Serial.println(F(" - the server didn't respond within the keepalive time")); 
     break;
   case -3:
     Serial.println(F(" - the network connection was broken")); 
     break;
   case -2:
     Serial.println(F(" - the network connection failed")); 
     break;
   case -1:
     Serial.println(F(" - the client is disconnected cleanly")); 
     break;
   case 0:
     Serial.println(F(" - the client is connected")); 
     break;
   case 1:
     Serial.println(F(" - the server doesn't support the requested version of MQTT")); 
     break;
   case 2:
     Serial.println(F(" - the server rejected the client identifier")); 
     break;
   case 3:
     Serial.println(F(" - the server was unable to accept the connection")); 
     break;
   case 4:
     Serial.println(F("  - the username/password were rejected")); 
     break;
   case 5:
     Serial.println(F(" - the client was not authorized to connect")); 
     break;   
   default:
     Serial.println(F(" - unknown state")); 
     break;
  }  
}

void tempFunction() // Function tempFunction reads temperature from DS18B20 sensor and sends it to Domoticz via MQTT protocol
{
  Temperature = tempReading(); // Current temperature is measured

	#if defined DEBUG
	  Serial.print(F("Current time is: "));
	  Serial.println(now());
	#endif

  // Read temperature is printed
  Serial.print(F("Temperature: "));
  Serial.print(Temperature);
  Serial.println(F("DegC"));

  //Send data to MQTT broker running on a Raspberry Pi
  sendMQTTPayload(createMQTTPayload(temperatureSensorIDX));
}

float tempReading() // Function tempReading reads temperature from DS18B20 sensor and returns it
{
  sensors.requestTemperatures(); // Send the command to get temperatures
  return (float) (sensors.getTempCByIndex(0)); // Return measured temperature
}

// function to print a DS18B20 device address
void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}
