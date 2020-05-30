#include <Arduino.h>
#include "secrets.h"
#include <WiFiClientSecure.h>
#include <MQTTClient.h>
#include <ArduinoJson.h>
#include "WiFi.h"

#include <Adafruit_Sensor.h>
#include <Adafruit_AM2320.h>

// The MQTT topics that this device should publish/subscribe
#define AWS_IOT_PUBLISH_TOPIC   "esp32/pub"
#define AWS_IOT_SUBSCRIBE_TOPIC "esp32/sub"

#define I2C_SDA 33
#define I2C_SCL 32

WiFiClientSecure net = WiFiClientSecure();
MQTTClient client = MQTTClient(256);

TwoWire myTwoWire = TwoWire(0);
Adafruit_AM2320 am2320;

void messageHandler(String &topic, String &payload) {
  Serial.println("incoming: " + topic + " - " + payload);
}

void connectAWS()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.println("Connecting to Wi-Fi");

  while (WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }

  // Configure WiFiClientSecure to use the AWS IoT device credentials
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);

  // Connect to the MQTT broker on the AWS endpoint we defined earlier
  client.begin(AWS_IOT_ENDPOINT, 8883, net);

  // Create a message handler
  client.onMessage(messageHandler);

  Serial.print("Connecting to AWS IOT");

  while (!client.connect(THINGNAME)) {
    Serial.print(".");
    delay(100);
  }

  if(!client.connected()){
    Serial.println("AWS IoT Timeout!");
    return;
  }

  // Subscribe to a topic
  client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);

  Serial.println("AWS IoT Connected!");
}

void publishMessage()
{
  StaticJsonDocument<200> doc;
  doc["time"] = millis();
  doc["temperature"] = am2320.readTemperature();
  doc["humidity"] = am2320.readHumidity();
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer); // print to client

  client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
}

void setup() {
  Serial.begin(9600);
  
  myTwoWire.begin(I2C_SDA, I2C_SCL, 100000);
  am2320 = Adafruit_AM2320(&myTwoWire, -1, -1);
  am2320.begin();

  delay(2000);

  connectAWS();
}

void loop() {
  publishMessage();
  client.loop();
  delay(1000);
}
