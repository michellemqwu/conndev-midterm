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

float prevX, prevY, prevZ;
int prevSensorReading;

float acceXThreshold = 0.03;
float acceYThreshold = 0.03;
float acceZThreshold = 0.03;
int fsrThreshold = 50;

int timeZone = -5;

int sameReading = 0;
int readingThreshold = 20;

int wifiLEDPin = 2;
int mqttLEDPin = 3;
int fsrPin = 7;

void setup() {
  Serial.begin(9600);
  while (!Serial)
    ;

  pinMode(wifiLEDPin, OUTPUT);
  pinMode(mqttLEDPin, OUTPUT);
  pinMode(fsrPin, OUTPUT);
  rtc.begin();

  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");
    while (1)
      ;
  }

  connectToNetwork();
  mqttClient.setId(clientID);
  mqttClient.setUsernamePassword(SECRET_MQTT_USER, SECRET_MQTT_PASS);

  unsigned long epoch;
  do {
    Serial.println("Trying to get time...");
    epoch = WiFi.getTime();
    delay(500);
  } while (epoch == 0);
  Serial.print("Epoch received: ");
  Serial.println(epoch);
  rtc.setEpoch(epoch);

  int dayOfWeek = ((epoch / 86400) + 4) % 7;

  if (daylightSavings(dayOfWeek)) {
    timeZone = timeZone + 1;
  }

  Serial.println(timeZone);
  rtc.setHours(rtc.getHours() + timeZone);

  Serial.println(getTimeString(rtc.getHours(),
                               rtc.getMinutes(),
                               rtc.getSeconds()));
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

  String jsonMessage = "{\"fsr\": FSR, \"timestamp\": TIMESTAMP, \"stable\": STABLE,\"acce\": {\"acceX\": ACCEX, \"acceY\": ACCEY, \"acceZ\": ACCEZ}}";
  float x, y, z;
  int sensorReading = analogRead(7);

  if (IMU.accelerationAvailable()) {
    IMU.readAcceleration(x, y, z);

    if (withinThreshold(x, prevX, acceXThreshold) && withinThreshold(y, prevY, acceYThreshold) && withinThreshold(z, prevZ, acceZThreshold) && withinThreshold(sensorReading, prevSensorReading, fsrThreshold)) {
      sameReading++;
      if (sameReading == readingThreshold) {
        Serial.println("stable!");
        jsonMessage.replace("STABLE", String(1));
        sameReading = 0;
      }
    } else {
      sameReading = 0;
      jsonMessage.replace("STABLE", String(0));
    }
    Serial.println("send message!");
    String timestamp = getTimeString(rtc.getHours(), rtc.getMinutes(), rtc.getSeconds());
    timestamp = timestamp + " " + String(rtc.getMonth()) + "/" + String(rtc.getDay()) + "/" + String(rtc.getYear());
    jsonMessage.replace("TIMESTAMP", timestamp);
    jsonMessage.replace("FSR", String(sensorReading));
    jsonMessage.replace("ACCEX", String(x));
    jsonMessage.replace("ACCEY", String(y));
    jsonMessage.replace("ACCEZ", String(z));
    Serial.print(jsonMessage);
    if (mqttClient.connected()) {
      mqttClient.beginMessage(topic);
      mqttClient.print(jsonMessage);
      mqttClient.endMessage();
      Serial.print("published a message: ");
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
    blinking(mqttLEDPin);
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
    blinking(wifiLEDPin);
    //delay(2000);
  }
  Serial.print("Connected. My IP address: ");
  Serial.println(WiFi.localIP());
}

void blinking(int LEDPin) {
  for (int i = 0; i <= 5; i++) {
    digitalWrite(LEDPin, HIGH);
    delay(200);
    digitalWrite(LEDPin, LOW);
    delay(200);
  }
}

bool daylightSavings(int dow) {
  //January, february, and december are out.
  if (rtc.getMonth() < 3 || rtc.getMonth() > 11) {
    return false;
  }
  //April to October are in
  if (rtc.getMonth() > 3 && rtc.getMonth() < 11) {
    return true;
  }
  int previousSunday = rtc.getDay() - dow;
  //In march, we are DST if our previous sunday was on or after the 8th.
  if (rtc.getMonth() == 3) {
    return previousSunday >= 8;
  }
  //In november we must be before the first sunday to be dst.
  //That means the previous sunday must be before the 1st.
  return previousSunday <= 0;
}

String getTimeString(int h, int m, int s) {
  h = h - 20;
  String now = "";
  if (h < 0) now += "0";
  now += h;
  now += ":";
  if (m < 0) now += "0";
  now += m;
  now += ":";
  if (s < 0) now += "0";
  now += s;
  return now;
}
