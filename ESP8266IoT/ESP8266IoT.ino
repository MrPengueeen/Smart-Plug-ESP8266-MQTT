#include <ESP8266WiFi.h>
#include <PubSubClient.h>

const char* ssid = "ssid";
const char* password =  "password";
const char* mqttServer = "soldier.cloudmqtt.com";
const int mqttPort = 14966;
const char* mqttUser = "mqttuser";
const char* mqttPassword = "mqttpass";

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {

  pinMode(13, OUTPUT);
  digitalWrite(13, HIGH);
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

  client.publish("welcomeMessage", "Hello from ESP8266");
  client.subscribe("ledSwitch");


}

void callback(char* topic, byte* payload, unsigned int length) {

  payload[length] = '\0';
  String message = (char*)payload;

  if (strcmp(topic, "ledSwitch") == 0) {
    if (message[0] == '1') {
      digitalWrite(13, LOW);
    }
    else if (message[0] == '0') {
      digitalWrite(13, HIGH);
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
