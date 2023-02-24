#include <Arduino_LSM6DS3.h>
#include <RTCZero.h>
#include <WiFiNINA.h> 
#include <ArduinoMqttClient.h>

RTCZero rtc;
WiFiClient wifi;
MqttClient mqttClient(wifi);

float acceX, acceY, acceZ;
float gyroX, gyroY, gyroZ;

unsigned long epoch;

String jsonMessage = "{\"fsr\": FSR, \"timestamp\": TIMESTAMP, \"gyro\": {\"gyroX\": GYROX, \"gyroY\": GYROY, \"gyroZ\": GYROZ}, \"acce\": {\"acceX\": ACCEX, \"acceY\": ACCEY, \"acceZ\": ACCEZ}}";

void setup() {
  Serial.begin(9600);
  while (!Serial);

  pinMode(7, OUTPUT);

  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");
    while (1);
  }

  Serial.print("Gyroscope sample rate = ");
  Serial.print(IMU.gyroscopeSampleRate());
  Serial.println(" Hz");
  Serial.println();
  Serial.println("Gyroscope in degrees/second");
  Serial.println("X\tY\tZ");

  Serial.print("Accelerometer sample rate = ");
  Serial.print(IMU.accelerationSampleRate());
  Serial.println(" Hz");
  Serial.println();
  Serial.println("Acceleration in g's");
  Serial.println("X\tY\tZ");

  rtc.begin();
  rtc.setEpoch(1677268722);
}

void loop() {
  int sensorReading = analogRead(7);
  Serial.print("FSR: ");
  Serial.println(sensorReading);

  if (IMU.accelerationAvailable()) {
    IMU.readAcceleration(acceX, acceY, acceZ);
    Serial.print("acce:" );
    Serial.print(acceX);
    Serial.print('\t');
    Serial.print(acceY);
    Serial.print('\t');
    Serial.println(acceZ);
  }

  if (IMU.gyroscopeAvailable()) {
    IMU.readGyroscope(gyroX, gyroY, gyroZ);
    Serial.print("gyro:" );
    Serial.print(gyroX);
    Serial.print('\t');
    Serial.print(gyroY);
    Serial.print('\t');
    Serial.println(gyroZ);
  }

  epoch = rtc.getEpoch();
  Serial.print("time: ");
  Serial.println(epoch);

  // jsonMessage.replace("FSR", String(sensorReading));
  // jsonMessage.replace("TIMESTAMP", String(epoch));
  // jsonMessage.replace("GYROX", String(gyroX));
  // jsonMessage.replace("GYROY", String(gyroY));
  // jsonMessage.replace("GYROZ", String(gyroZ));
  // jsonMessage.replace("ACCEX", String(acceX));
  // jsonMessage.replace("ACCEY", String(acceY));
  // jsonMessage.replace("ACCEZ", String(acceZ));
  
  // Serial.println(jsonMessage);

  Serial.println("--------------------");
  delay(500);
}
