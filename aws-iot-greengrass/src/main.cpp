#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <WiFiClientSecure.h>
#include <AWSGreenGrassIoT.h>

#include <NTPClient.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_AM2320.h>

#include <ArduinoJson.h>

#include "secrets.h"

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "time1.google.com", 3600, 60000);

AWSGreenGrassIoT *greengrass;

int status = WL_IDLE_STATUS;

#define AWS_IOT_PUBLISH_TOPIC "sensors/am2320"

#define I2C_SDA 33
#define I2C_SCL 32

TwoWire myTwoWire = TwoWire(0);
Adafruit_AM2320 am2320;

void publishMessage()
{
  timeClient.update();

  StaticJsonDocument<200> doc;
  doc["time"] = timeClient.getEpochTime();
  doc["temperature"] = am2320.readTemperature();
  doc["humidity"] = am2320.readHumidity();
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer); // print to client

  if (greengrass->publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer))
  {
    Serial.print("Publish AM2320 Message:");
    Serial.println(jsonBuffer);
  }
  else
  {
    Serial.println("Publish AM2320 failed");
  }
}

void setup()
{
  Serial.begin(9600);

  myTwoWire.begin(I2C_SDA, I2C_SCL, 100000);
  am2320 = Adafruit_AM2320(&myTwoWire, -1, -1);
  am2320.begin();

  while (status != WL_CONNECTED)
  {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(WIFI_SSID);
    // Connect to WPA/WPA2 network.
    status = WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    // wait 3 seconds for connection:
    delay(3000);
  }

  Serial.println("Connected to wifi");

  greengrass = new AWSGreenGrassIoT(AWS_IOT_ENDPOINT, THINGNAME, AWS_CERT_CA, AWS_CERT_CRT, AWS_CERT_PRIVATE);

  timeClient.begin();

  if (greengrass->connectToIoTCore() == true)
  {
    Serial.println("Connected to AWS IoT core");
    delay(500);
    if (greengrass->connectToGG() == true)
    {
      Serial.println("Connected to AWS IoT Greengrass");
      delay(500);
    }
    else
    {
      Serial.println("Connection to AWS IoT Greengrass failed");
    }
  }
  else
  {
    Serial.println("Connection to AWS IoT Core failed");
  }
}

void loop()
{
  if (greengrass->isConnected())
  {
    publishMessage();
  }
  delay(2000);
}
