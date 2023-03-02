#include <WiFiNINA.h>
#include <ArduinoMqttClient.h>
#include "arduino_secrets.h"
#include <Arduino_LSM6DS3.h>
#include <RTCZero.h>

RTCZero rtc;
WiFiClient wifi;
MqttClient mqttClient(wifi);

char broker[] = "public.cloud.shiftr.io";
int port = 1883;
char topic[] = "conndev/mmw451";
char clientID[] = "arduinoClient-mmw451";

unsigned long epoch;

float prevX, prevY, prevZ;
int prevSensorReading;

float acceXThreshold = 0.03;
float acceYThreshold = 0.03;
float acceZThreshold = 0.03;
int fsrThreshold = 50;

int sameReading = 0;
int readingThreshold = 10;

void setup() {
  Serial.begin(9600);
  while (!Serial)
    ;

  pinMode(7, OUTPUT);

  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");
    while (1)
      ;
  }

  rtc.begin();
  rtc.setEpoch(1677268722);

  connectToNetwork();
  mqttClient.setId(clientID);
  mqttClient.setUsernamePassword(SECRET_MQTT_USER, SECRET_MQTT_PASS);
}

void loop() {

  if (WiFi.status() != WL_CONNECTED) {
    connectToNetwork();
    return;
  }

  if (!mqttClient.connected()) {
    Serial.println("attempting to connect to broker");
    connectToBroker();
  }

  mqttClient.poll();

  String jsonMessage = "{\"fsr\": FSR, \"timestamp\": TIMESTAMP, \"acce\": {\"acceX\": ACCEX, \"acceY\": ACCEY, \"acceZ\": ACCEZ}}";
  float x, y, z;
  int sensorReading = analogRead(7);
  
  if (IMU.accelerationAvailable()) {
    IMU.readAcceleration(x, y, z);

    if (withinThreshold(x, prevX, acceXThreshold) && withinThreshold(y, prevY, acceYThreshold) && withinThreshold(z, prevZ, acceZThreshold) && withinThreshold(sensorReading, prevSensorReading, fsrThreshold)) {
      sameReading++;
      if (sameReading == readingThreshold) {
        Serial.println("stable!");
        sameReading = 0;
      }
    } else {
      sameReading = 0;
      epoch = rtc.getEpoch();
      Serial.println("send message!");
      jsonMessage.replace("FSR", String(sensorReading));
      jsonMessage.replace("TIMESTAMP", String(epoch));
      jsonMessage.replace("ACCEX", String(x));
      jsonMessage.replace("ACCEY", String(y));
      jsonMessage.replace("ACCEZ", String(z));

      if (mqttClient.connected()) {
        mqttClient.beginMessage(topic);
        mqttClient.print(jsonMessage);
        mqttClient.endMessage();
        Serial.print("published a message: ");
        Serial.println(jsonMessage);
      }
    }
    prevX = x;
    prevY = y;
    prevZ = z;
  }

  prevSensorReading = sensorReading;
  Serial.println("--------------------");
  delay(40);
}

boolean withinThreshold(int current, int prev, int threshold) {
  if ((prev - threshold <= current) && (current <= prev + threshold)) {
    return true;
  }
  return false;
}

boolean connectToBroker() {
  if (!mqttClient.connect(broker, port)) {
    Serial.print("MOTT connection failed. Error no: ");
    Serial.println(mqttClient.connectError());
    return false;
  }

  mqttClient.onMessage(onMqttMessage);
  Serial.print("Subscribing to topic: ");
  Serial.println(topic);
  mqttClient.subscribe(topic);
  return true;
}

void onMqttMessage(int messageSize) {
  Serial.println("Received a message with topic ");
  Serial.print(mqttClient.messageTopic());
  Serial.print(", length ");
  Serial.print(messageSize);
  Serial.println(" bytes:");
  String incoming = "";
  while (mqttClient.available()) {
    incoming += (char)mqttClient.read();
  }
  int result = incoming.toInt();
  if (result > 0) {
    analogWrite(LED_BUILTIN, result);
  }
  Serial.println(result);
}

void connectToNetwork() {
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("Attempting to connect to: " + String(SECRET_SSID));
    WiFi.begin(SECRET_SSID, SECRET_PASS);
    delay(2000);
  }
  Serial.print("Connected. My IP address: ");
  Serial.println(WiFi.localIP());
}
