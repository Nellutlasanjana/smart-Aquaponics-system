#include <ArduinoWiFiServer.h>
#include <BearSSLHelpers.h>
#include <CertStoreBearSSL.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiAP.h>
#include <ESP8266WiFiGeneric.h>
#include <ESP8266WiFiGratuitous.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266WiFiSTA.h>
#include <ESP8266WiFiScan.h>
#include <ESP8266WiFiType.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <WiFiClientSecureBearSSL.h>
#include <WiFiServer.h>
#include <WiFiServerSecure.h>
#include <WiFiServerSecureBearSSL.h>
#include <WiFiUdp.h>

#include <ESP8266HTTPClient.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

// Wi-Fi Credentials
const char* ssid = "vivo Y22";          // Your Wi-Fi SSID
const char* password = "1234567890";  // Your Wi-Fi password

// ThingSpeak credentials
const char* thingSpeakHost = "api.thingspeak.com";
const char* thingSpeakApiKey = "NRLTMA1LRMWR8MZR";
#define MPU6050_SDA 21 // GPIO 21 for SDA
#define MPU6050_SCL 22 // GPIO 22 for SCL

// Initialize MPU6050
Adafruit_MPU6050 mpu;

// ESP32 Web Server on port 80
WiFiServer server(80);

void setup() {
  Serial.begin(115200);
  Wire.begin();

  // Initialize MPU6050
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }
  
  // Set accelerometer and gyro ranges for MPU6050
  mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
  mpu.setGyroRange(MPU6050_RANGE_250_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  Serial.println("MPU6050 initialized.");

  // Connect to Wi-Fi
  Serial.print("Connecting to Wi-Fi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected!");

  // Print the IP address assigned to the ESP32
  Serial.print("ESP32 IP Address: ");
  Serial.println(WiFi.localIP());

  // Start the server
  server.begin();
  Serial.println("Server started.");
}

void loop() {
  // Check if a client has connected
  WiFiClient client = server.available();
  if (client) {
    Serial.println("New Client connected.");

    // Wait until the client sends data
    while (client.connected() && !client.available()) {
      delay(1);
    }

    // Read MPU6050 data
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    // Send data to ThingSpeak
    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;

      // Declare a WiFiClient object
      WiFiClient client;

      // Construct the ThingSpeak URL with the field data
      String url = String("http://") + thingSpeakHost + "/update?api_key=" + thingSpeakApiKey;
      url += "&field1=" + String(a.acceleration.x);
      url += "&field2=" + String(a.acceleration.y);
      url += "&field3=" + String(a.acceleration.z);
      url += "&field4=" + String(g.gyro.x);
      url += "&field5=" + String(g.gyro.y);
      url += "&field6=" + String(g.gyro.z);

      // Use the new API: http.begin(client, url);
      http.begin(client, url);
      int httpResponseCode = http.GET();

      // Check for successful response
      if (httpResponseCode > 0) {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
      } else {
        Serial.print("Error in HTTP request: ");
        Serial.println(httpResponseCode);
      }

      http.end();  // Free resources
    }

    // Send response back to the client
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Connection: close");
    client.println();

    // Prepare JSON data (if needed for the client)
    String jsonData = "{";
    jsonData += "\"accel_x\":" + String(a.acceleration.x) + ",";
    jsonData += "\"accel_y\":" + String(a.acceleration.y) + ",";
    jsonData += "\"accel_z\":" + String(a.acceleration.z) + ",";
    jsonData += "\"gyro_x\":" + String(g.gyro.x) + ",";
    jsonData += "\"gyro_y\":" + String(g.gyro.y) + ",";
    jsonData += "\"gyro_z\":" + String(g.gyro.z);
    jsonData += "}";

    client.println(jsonData);

    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
  }

  delay(100);  // Adjust delay as needed
}

