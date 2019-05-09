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

const int led_error = 7;
const int led_mqtt = 1;

char *topic;

/* Create an rtc object */
RTCZero rtc;

const byte hours = 0;
const byte minutes = 0;
const byte seconds = 0;
bool matched = false;

void connect_wifi() {
  // attempt to connect to Wifi network:
//  blink(LED_BUILTIN);
  WiFi.noLowPowerMode();
  Serial.print("Attempting to connect to WPA SSID: ");
  Serial.println(ssid);
  while (WiFi.begin(ssid, pass) != WL_CONNECTED) {
    // failed, retry
    Serial.print(".");
    blink(LED_BUILTIN);
  }
  Serial.println("You're connected to the network");
}

void disconnect_wifi(){
//  blink(LED_BUILTIN);
  WiFi.disconnect();
  WiFi.end();
  Serial.println("Wifi disconnected");
}

int setup_mqtt() {
  mqttClient.setId("mkr1000");
  mqttClient.setUsernamePassword(mqtt_user, mqtt_pass);
}

boolean connect_mqtt(){
//  blink(led_mqtt);
  int attemps = 0;
  while (attemps < 3 && !mqttClient.connected()) {
    blink(led_error);
    mqttClient.connect(broker, port);
    attemps = attemps + 1;
  }
  return mqttClient.connected();
}

void disconnect_mqtt(){
//  blink(led_mqtt);
  //mqttClient.disconnect();
}

void send_metric_mqtt(char *topic, float msg) {
  Serial.println("Sending metric");
  mqttClient.beginMessage(topic);
  mqttClient.print(msg);
  int exit_code = mqttClient.endMessage();

  if (exit_code = 0){
    Serial.println("Sent");
    blink(led_mqtt);
  }
  else{
    Serial.println("Error sending the message");
    blink(led_error);
  }
}

void send_msg_mqtt(char *topic, char *msg) {
  Serial.println("Sending message");
  mqttClient.beginMessage(topic);
  mqttClient.print(msg);
  int exit_code = mqttClient.endMessage();
  if (exit_code = 0){
    Serial.println("Sent");
    blink(led_mqtt);
  }
  else{
    Serial.println("Error sending the message");
    blink(led_error);
  }
  
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

void blink(int x){
  digitalWrite(x, HIGH);
  delay(1000);
  digitalWrite(x, LOW);
  delay(1000);
}

void setup() {
  Serial.begin(9600);
  setup_mqtt();
  setup_bme280();

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(led_error, OUTPUT);
  pinMode(led_mqtt, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  digitalWrite(led_error, LOW);
  digitalWrite(led_mqtt, LOW);

  rtc.begin();
  rtc.setTime(hours, minutes, seconds);
  rtc.setAlarmTime(0, 15, 0);
  rtc.enableAlarm(rtc.MATCH_MMSS);

  rtc.attachInterrupt(alarmMatch);
  rtc.standbyMode();
}

void loop() {
  
  if (matched) {
      matched = false;
      blink(LED_BUILTIN);
      
      connect_wifi();
      if (connect_mqtt()){
        send_metric_mqtt(topic = "tmp", bme.readTemperature());
      }
      
      disconnect_mqtt();
      disconnect_wifi();
      
      rtc.setTime(hours, minutes, seconds);
  }
  
  rtc.standbyMode();
}


void alarmMatch()
{
  //send_metric_mqtt(topic = "tmp", bme.readTemperature());
  //send_metric_mqtt(topic = "hum", bme.readHumidity());
  //send_metric_mqtt(topic = "pre", bme.readPressure() / 100.0F);
  matched = true;
}
