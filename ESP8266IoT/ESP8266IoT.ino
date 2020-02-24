//Include necessary libraries
#include <ArduinoJson.h>
#include <Wire.h>
#include "RTClib.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <EEPROM.h>


RTC_DS1307 rtc; // Object declaration for RTC


// WiFi and MQTT Broker credentials
const char* ssid = "";
const char* password =  "";
const char* mqttServer = "mqtt.beebotte.com";
const int mqttPort = 1883;
const char* mqttUser = "";
const char* mqttPassword = "";


char json[200]; // Sample string for incoming JSON data

// Flags for retaining previous plug and timer status
int lastPlugStatus;
int lastTimerStatus = 1;


// WiFi and MQTT object declaration
WiFiClient espClient;
PubSubClient client(espClient);


void setup() {

  pinMode(13, OUTPUT);
  Serial.begin(115200);
  Wire.begin();
  EEPROM.begin(512);

  WiFi.begin(ssid, password); //Initiate WiFi


  //Check if the module is able to connect to the specified network
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to the WiFi network");

  //MQTT Setup
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);

  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");

    if (client.connect("ESP8266Client", mqttUser, mqttPassword )) {

      Serial.println("connected");

    } else {

      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);

    }
  }

  // RTC Setup
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running!");
  }

  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));  //Set compilation time as the time for RTC. Comment this line out after setting the time once and reupload the sketch

  //EEPROM.put(3, 1);
  //EEPROM.commit();

  /* Read last status of plug and timer from EEPROM if there
      has been any power outage
  */
  lastPlugStatus = (int)EEPROM.read(4);
  lastTimerStatus = (int)EEPROM.read(3);

  //Retain last plug status and unsuccessful timer
  retainLastPlugStatus();
  retainLastTimerStatus();

  //Subscribe to topics and publish startup message
  client.publish("Esp8266ioT/messages", "Hello from Smart Plug!");
  client.subscribe("Esp8266ioT/ga");
  client.subscribe("Esp8266ioT/toggle");
  client.subscribe("Esp8266ioT/time");

  Serial.print("Last Plug Status : ");
  Serial.println(lastPlugStatus);


}


//This is where everything happens
//Is called when a message arrives to any subscribed topic

void callback(char* topic, byte* payload, unsigned int length) {

  StaticJsonBuffer<200> jsonBuffer;  //Start JSON
  payload[length] = '\0'; //Remove excess bytes from payload string


  //Make another copy of payload in the string JSON[]
  for (int i = 0; i < length; i++) {
    //Serial.print((char)payload[i]);
    json[i] = (char)payload[i];
    //Serial.print(payload[i]);
  }


  //Check which topic the payload was published to and act accordingly

  // Topic : Esp8266ioT/time  (Timer)
  if (strcmp(topic, "Esp8266ioT/time") == 0) {
    checkTimer(json);
  }


  //Topic : Esp8266ioT/ga (Google Assistant)
  if (strcmp(topic, "Esp8266ioT/ga") == 0) {

    //Parse the JSON Object
    JsonObject& root = jsonBuffer.parseObject(json);
    String data = root["data"];

    if (data == "1") {
      digitalWrite(13, HIGH);
      EEPROM.put(4, 1); // Put the plug status in EEPROM so that it can be retained again after power outage of any sort
      EEPROM.commit();
      lastPlugStatus = (int)EEPROM.read(4);
      client.publish("Esp8266ioT/status", "Plug is ON!");
    }
    else if (data == "0") {
      digitalWrite(13, LOW);
      EEPROM.put(4, 0);
      EEPROM.commit();
      lastPlugStatus = (int)EEPROM.read(4);
      client.publish("Esp8266ioT/status", "Plug is OFF!");
    }

  }

  String message = (char*)payload;


  // Topic : Esp8266ioT/toggle (Toggle Switch)
  if (strcmp(topic, "Esp8266ioT/toggle") == 0) {
    if (message == "1") {
      digitalWrite(13, HIGH);
      EEPROM.put(4, 1);
      EEPROM.commit();
      lastPlugStatus = (int)EEPROM.read(4);
      client.publish("Esp8266ioT/status", "Plug is ON!");
    }
    else if (message == "0") {
      digitalWrite(13, LOW);
      EEPROM.put(4, 0);
      EEPROM.commit();
      lastPlugStatus = (int)EEPROM.read(4);
      client.publish("Esp8266ioT/status", "Plug is OFF!");
    }

  }

  Serial.print("Message arrived in topic: ");
  Serial.println(topic);

  Serial.print("Message:");
  Serial.println(message);



  Serial.println();
  Serial.println("-----------------------");

}


void loop() {

  client.loop();

}

// The timer function that handles the timer feature
// Takes the JSON string as input
void checkTimer(char *json) {
  client.publish("Esp8266ioT/messages", "Timer has been set!"); //Confirmation message


  char *hour;
  char *minute;
  char delim[] = ":";


  json[5] = '\0';

  //Breaks the "hh:mm" formatted string into two parts. One taking the part before ':' and another taking the part after ':'
  hour = strtok(json, delim);
  minute = strtok(NULL, delim);


  //Typecasting the hour and minute strings into integers
  int Hour = atoi(hour);
  int Minute = atoi(minute);

  Serial.print("Switching the plug at ");
  Serial.print(Hour);
  Serial.print(":");
  Serial.println(Minute);

  //Keeps the set time in EEPROM for reatining afterwards
  EEPROM.put(0, Hour);
  EEPROM.put(1, Minute);
  EEPROM.put(3, 0);
  EEPROM.commit();
  lastTimerStatus = (int)EEPROM.read(3); //Flag variable for retaining any unsuccessful timer due to power outage
  int check = (int)EEPROM.read(0);
  int check2 = (int)EEPROM.read(1);

  Serial.println("EEPROM TIME   ");
  Serial.print(check);
  Serial.print(":");
  Serial.println(check2);

  //Constantly checks if the current time from the RTC matches the set time
  while (1) {
    DateTime now = rtc.now();
    int hr =  (int)(now.hour());
    int mn = (int)(now.minute());

    //Converts the current and set time into minutes
    int current = (hr * 60) + mn;
    int setTime = (check * 60) + check2;

    Serial.print("Current Time : ");
    Serial.print(hr);
    Serial.print(":");
    Serial.println(mn);


    Serial.print("Set time : ");
    Serial.print(check);
    Serial.print(":");
    Serial.println(check2);

    //Checks if the set time has passed already
    if (current > setTime) {

      Serial.println("The set time has passed already! ");
      client.publish("Esp8266ioT/messages", "Set time exceeded current time!");

      break; //breaks the loop
    }

    //True if the current time matches with the set time
    //Toggles the plug. Meaning, if the Plug was ON before, this will turn it OFF and vice versa
    if (hr == check && mn == check2) {
      if (lastPlugStatus == 0) {
        digitalWrite(13, HIGH);
        EEPROM.put(4, 1);
        EEPROM.commit();
        lastPlugStatus = (int)EEPROM.read(4);
        Serial.println("Done!");
        client.publish("Esp8266ioT/status", "Plug is ON!");
        break;
      } else if (lastPlugStatus == 1) {

        digitalWrite(13, LOW);
        EEPROM.put(4, 0);
        EEPROM.commit();
        lastPlugStatus = (int)EEPROM.read(4);
        Serial.println("Done!");
        client.publish("Esp8266ioT/status", "Plug is OFF!");
        break;

      }
    }
    yield();  //EXTREMELY IMPORTANT! This gives the ESP8266 the time to get it's important internal processes to run.
  }


  Serial.print("Mqtt status: ");
  Serial.println(client.connected());
  Serial.print("Wifi status: ");
  Serial.println(WiFi.status());

  reconnect();  //Reconnect to WiFi and MQTT

  EEPROM.put(3, 1);
  EEPROM.commit();
  lastTimerStatus = (int)EEPROM.read(3);

  if (lastPlugStatus) {
    client.publish("Esp8266ioT/status", "Plug is ON!");
  } else if (!lastPlugStatus) {
    client.publish("Esp8266ioT/status", "Plug is OFF!");
  }

  client.publish("Esp8266ioT/messages", "Toggle successful!");
}

//Function to reconnect to WiFi and MQTT Broker
void reconnect() {

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to the WiFi network");

  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);

  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");

    if (client.connect("ESP8266Client", mqttUser, mqttPassword )) {

      Serial.println("connected");

      client.publish("Esp8266ioT/messages", "Reconnected!");
      client.subscribe("Esp8266ioT/ga");
      client.subscribe("Esp8266ioT/messages");
      client.subscribe("Esp8266ioT/toggle");
      client.subscribe("Esp8266ioT/time");



    } else {

      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);

    }
  }

}


//Function to retain last plug status after every restart
void retainLastPlugStatus() {

  if (lastPlugStatus == 0) {
    digitalWrite(13, LOW);
    EEPROM.put(4, 0);
    EEPROM.commit();
    lastPlugStatus = (int)EEPROM.read(4);
    client.publish("Esp8266ioT/status", "Plug is OFF!");
  } else if (lastPlugStatus == 1) {
    digitalWrite(13, HIGH);
    EEPROM.put(4, 1);
    EEPROM.commit();
    lastPlugStatus = (int)EEPROM.read(4);
    client.publish("Esp8266ioT/status", "Plug is ON!");
  }

  client.publish("Esp8266ioT/messages", "Retaining successful!");

}

//Function to retain any unsuccessful timer after restart
void retainLastTimerStatus() {

  if (lastTimerStatus == 0) {


    int check = (int)EEPROM.read(0);
    int check2 = (int)EEPROM.read(1);

    client.publish("Esp8266ioT/messages", "Timer retain successful!");
    Serial.print("Switching the plug at ");
    Serial.print(check);
    Serial.print(":");
    Serial.println(check2);

    Serial.println("EEPROM TIME   ");
    Serial.print(check);
    Serial.print(":");
    Serial.println(check2);

    while (1) {
      DateTime now = rtc.now();
      int hr =  (int)(now.hour());
      int mn = (int)(now.minute());

      int current = (hr * 60) + mn;
      int setTime = (check * 60) + check2;

      Serial.print("Current Time : ");
      Serial.print(hr);
      Serial.print(":");
      Serial.println(mn);


      Serial.print("Set time : ");
      Serial.print(check);
      Serial.print(":");
      Serial.println(check2);

      if (current > setTime) {

        Serial.println("The set time has passed already! ");
        client.publish("Esp8266ioT/messages", "Set time exceeded current time!");

        break;
      }

      if (hr == check && mn == check2) {
        if (lastPlugStatus == 0) {
          digitalWrite(13, HIGH);
          EEPROM.put(4, 1);
          EEPROM.commit();
          lastPlugStatus = (int)EEPROM.read(4);
          Serial.println("Done!");
          client.publish("Esp8266ioT/status", "Plug is ON!");
          break;
        } else if (lastPlugStatus == 1) {

          digitalWrite(13, LOW);
          EEPROM.put(4, 0);
          EEPROM.commit();
          lastPlugStatus = (int)EEPROM.read(4);
          Serial.println("Done!");
          client.publish("Esp8266ioT/status", "Plug is OFF!");

          break;

        }
      }
      yield();
    }


    Serial.print("Mqtt status: ");
    Serial.println(client.connected());
    Serial.print("Wifi status: ");
    Serial.println(WiFi.status());

    reconnect();
    EEPROM.put(3, 1);
    EEPROM.commit();
    lastTimerStatus = (int)EEPROM.read(3);


    if (lastPlugStatus) {
      client.publish("Esp8266ioT/status", "Plug is ON!");
    } else if (!lastPlugStatus) {
      client.publish("Esp8266ioT/status", "Plug is OFF!");
    }

    client.publish("Esp8266ioT/messages", "Toggle successful!");

  }
}
