#include <ESP8266WiFi.h>
#include <PubSubClient.h>

const char* ssid = "-----";
const char* password =  "-----";
const char* mqttServer = "soldier.cloudmqtt.com";
const int mqttPort = 14966;
const char* mqttUser = "-----";
const char* mqttPassword = "-----";

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {

  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);
  Serial.begin(115200);

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

  client.publish("ledSwitch", "Hello from ESP8266");
  client.subscribe("ledSwitch");

}

void callback(char* topic, byte* payload, unsigned int length) {

  Serial.print("Message arrived in topic: ");
  Serial.println(topic);

  Serial.print("Message:");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }

  Serial.println();
  Serial.println("-----------------------");

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(13, HIGH);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is active low on the ESP-01)
    //client.publish("ledSwitch", "LED is turned on!");

  } else {
    digitalWrite(13, LOW);  // Turn the LED off by making the voltage HIGH
    //client.publish("ledSwitch", "LED is turned off!");
  }

}

void loop() {
  client.loop();
}
