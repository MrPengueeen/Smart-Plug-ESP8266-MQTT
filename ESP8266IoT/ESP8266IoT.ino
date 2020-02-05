#include <ArduinoJson.h>
#include <Wire.h>
#include "RTClib.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <EEPROM.h>
RTC_DS1307 rtc;

const char* ssid = "Redmi a";
const char* password =  "06012019";
const char* mqttServer = "mqtt.beebotte.com";
const int mqttPort = 1883;
const char* mqttUser = "token_nq0Nher2unrIiXid";
const char* mqttPassword = "";
char json[200];


int lastPlugStatus;
int lastTimerStatus = 1;

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {

  pinMode(13, OUTPUT);
  //digitalWrite(13, HIGH);
  Serial.begin(115200);
  Wire.begin();
  EEPROM.begin(512);

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

    } else {

      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);

    }
  }


  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running!");
  }
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  //EEPROM.put(3, 1);
  //EEPROM.commit();
  lastPlugStatus = (int)EEPROM.read(4);
  lastTimerStatus = (int)EEPROM.read(3);
  retainLastPlugStatus();
  retainLastTimerStatus();

  client.publish("Esp8266ioT/messages", "Hello from Smart Plug!");
  client.subscribe("Esp8266ioT/ga");
  client.subscribe("Esp8266ioT/toggle");
  client.subscribe("Esp8266ioT/time");

  Serial.print("Last Plug Status : ");
  Serial.println(lastPlugStatus);


}

void callback(char* topic, byte* payload, unsigned int length) {

  StaticJsonBuffer<200> jsonBuffer;

  payload[length] = '\0';



  for (int i = 0; i < length; i++) {
    //Serial.print((char)payload[i]);
    json[i] = (char)payload[i];
    //Serial.print(payload[i]);
  }


 
  if (strcmp(topic, "Esp8266ioT/time") == 0) {
    checkTimer(json);
  }


  if (strcmp(topic, "Esp8266ioT/ga") == 0) {
    JsonObject& root = jsonBuffer.parseObject(json);
      String data = root["data"];

    if (data == "1") {
      digitalWrite(13, HIGH);
      EEPROM.put(4, 1);
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


void checkTimer(char *json) {
  client.publish("Esp8266ioT/messages", "Timer has been set!");


  char *hour;
  char *minute;
  char delim[] = ":";


  json[5] = '\0';

  hour = strtok(json, delim);
  minute = strtok(NULL, delim);

  int Hour = atoi(hour);
  int Minute = atoi(minute);

  Serial.print("Switching the plug at ");
  Serial.print(Hour);
  Serial.print(":");
  Serial.println(Minute);
  EEPROM.put(0, Hour);
  EEPROM.put(1, Minute);
  EEPROM.put(3, 0);
  EEPROM.commit();
  lastTimerStatus = (int)EEPROM.read(3);
  int check = (int)EEPROM.read(0);
  int check2 = (int)EEPROM.read(1);

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
