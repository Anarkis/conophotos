#include <ArduinoMqttClient.h>
#include <WiFi101.h>
#include "arduino_secrets.h"
#include <typeinfo>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <RTCZero.h>

#define BME_SCK 13
#define BME_MISO 12
#define BME_MOSI 11
#define BME_CS 10
#define SEALEVELPRESSURE_HPA (1013.25)

Adafruit_BME280 bme;

char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)

char mqtt_user[] = MQTT_USER;
char mqtt_pass[] = MQTT_PASS;

// To connect with SSL/TLS:
// 1) Change WiFiClient to WiFiSSLClient.
// 2) Change port value from 1883 to 8883.
// 3) Change broker value to a server with a known SSL/TLS root certificate
//    flashed in the WiFi module.

WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

const char broker[] = MQTT_IP;
int port = MQTT_PORT;

char *topic;

/* Create an rtc object */
RTCZero rtc;

const byte hours = 00;
const byte minutes = 00;
const byte seconds = 00;
const byte day = 17;
const byte month = 05;
const byte year = 19;

void setup_wifi() {
  // attempt to connect to Wifi network:
  Serial.print("Attempting to connect to WPA SSID: ");
  Serial.println(ssid);
  while (WiFi.begin(ssid, pass) != WL_CONNECTED) {
    // failed, retry
    Serial.print(".");
    delay(5000);
  }

  Serial.println("You're connected to the network");
}

int setup_mqtt() {
  mqttClient.setId("mkr1000");
  mqttClient.setUsernamePassword(mqtt_user, mqtt_pass);

  Serial.print("Attempting to connect to the MQTT broker: ");
  Serial.println(broker);

  if (!mqttClient.connect(broker, port)) {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.connectError());
    return mqttClient.connectError();
    //while (1);
  }

  Serial.println("You're connected to the MQTT broker!");
}

void setup_bme280() {
  bool status;
  // (you can also pass in a Wire library object like &Wire2)
  status = bme.begin();
  if (!status) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1);
  }

  Serial.println("-- Weather Station Scenario --");
  Serial.println("forced mode, 1x temperature / 1x humidity / 1x pressure oversampling,");
  Serial.println("filter off");
  bme.setSampling(Adafruit_BME280::MODE_FORCED,
                  Adafruit_BME280::SAMPLING_X1, // temperature
                  Adafruit_BME280::SAMPLING_X1, // pressure
                  Adafruit_BME280::SAMPLING_X1, // humidity
                  Adafruit_BME280::FILTER_OFF   );

}


void send_metric_mqtt(char *topic, float msg) {
  Serial.println("Sending message");
  mqttClient.beginMessage(topic);
  mqttClient.print(msg);
  mqttClient.endMessage();
  Serial.println("Sent");
}

void send_msg_mqtt(char *topic, char *msg) {
  Serial.println("Sending message");
  mqttClient.beginMessage(topic);
  mqttClient.print(msg);
  mqttClient.endMessage();
  Serial.println("Sent");
}

void setup() {
  Serial.begin(9600);
  setup_wifi();
  setup_mqtt();
  setup_bme280();

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  rtc.begin();
  rtc.setTime(hours, minutes, seconds);
  rtc.setDate(day, month, year);
  rtc.setAlarmTime(00, 05, 00);
  rtc.enableAlarm(rtc.MATCH_MMSS);

  rtc.attachInterrupt(alarmMatch);
  rtc.standbyMode();
}

void loop() {
  rtc.standbyMode();
}


void alarmMatch()
{
  send_metric_mqtt(topic = "tmp", bme.readTemperature());
  send_metric_mqtt(topic = "hum", bme.readHumidity());
  send_metric_mqtt(topic = "pre", bme.readPressure() / 100.0F);
  rtc.setTime(00,00,00);
}
