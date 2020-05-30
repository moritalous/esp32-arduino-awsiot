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

#include <esp_sleep.h>

#define AWS_IOT_PUBLISH_TOPIC "esp32/am2320"

#define I2C_SDA 33
#define I2C_SCL 32

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "time1.google.com", 3600, 60000);

AWSGreenGrassIoT *greengrass;

int status = WL_IDLE_STATUS;

TwoWire myTwoWire = TwoWire(0);
Adafruit_AM2320 am2320;

int count = 0;

void enterDeepSleep()
{
  Serial.println("enter deepSleep");
  esp_sleep_enable_timer_wakeup(10 * 1000 * 1000);
  esp_deep_sleep_start();
}

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

  while (status != WL_CONNECTED)
  {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(WIFI_SSID);
    // Connect to WPA/WPA2 network.
    status = WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    // wait 3 seconds for connection:
    delay(1000);
  }

  Serial.println("Connected to wifi");

  greengrass = new AWSGreenGrassIoT(AWS_IOT_ENDPOINT, THINGNAME, AWS_CERT_CA, AWS_CERT_CRT, AWS_CERT_PRIVATE);

  timeClient.begin();

  if (greengrass->connectToGG() == true)
  {
    Serial.println("Connected to AWS IoT Greengrass");
    delay(500);
  }
  else
  {
    Serial.println("Connection to AWS IoT Greengrass failed");
    enterDeepSleep();
  }

  myTwoWire.begin(I2C_SDA, I2C_SCL, 100000);
  pinMode(I2C_SDA, INPUT);
  pinMode(I2C_SCL, INPUT);

  am2320 = Adafruit_AM2320(&myTwoWire, -1, -1);
  am2320.begin();

  delay(1000);
}

void loop()
{
  if (greengrass->isConnected())
  {
    count++;
    publishMessage();
  }
  else
  {
    enterDeepSleep();
  }
  if (count > 2000)
  {
    enterDeepSleep();
  }
  delay(2000);
}
