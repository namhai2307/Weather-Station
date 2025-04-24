#include <WiFi.h>
#include <WebServer.h>
#include <BH1750.h>
#include <Wire.h>
#include <BME280I2C.h>
#include <Adafruit_BMP280.h>
#include "Adafruit_SHT4x.h"
#include <ESP32Servo.h>
#include <WiFiClient.h>
#include <ESPmDNS.h>
#include <Update.h>

//Setup for the solar tracker
#define LDR1 34
#define LDR2 35
//Define the error value
#define error 140
//Starting point of the servo motor
int Spoint =  90;
//Create an object for the servo motor
Servo servo;

//Pressure sensor setup
Adafruit_BMP280 bmp;

//Rain gauge setup
#define RAIN_GAUGE_PIN 18  // The pin connected to the hall sensor (digital input)
#define mili_liter_per_swing 2.475
#define six_hours 21600000 // 6 hours in milliseconds 21600000
unsigned long lastMeasurementTime_2 = 0; // To track the last measurement time
volatile int swingCount = 0; // To count the number of holes


// Encoder setup
#define ENCODER_PIN 23  // The pin connected to the encoder (digital input)
#define INTERVAL 2000 // Measurement interval in milliseconds (2 seconds)
volatile int holeCount = 0; // To count the number of holes
unsigned long lastMeasurementTime_1 = 0; // To track the last measurement time
float omega = 0.0; // To store calculated RPM

//LUX sensor setup
BH1750 lightMeter(0x23);

//Temperature and humidity sensor setup
Adafruit_SHT4x sht4 = Adafruit_SHT4x();

// WiFi credentials
const char *ssid = "Vodafone2.4G-B8F46";
const char *password = "Eb3NRfpdAXpcMz24";

// Create a web server on port 80
WebServer server(80);

// Timing variables
unsigned long   lastSensorReadTime = 0;
const unsigned long sensorReadInterval = 10000; // 2 seconds

// Sensor values
float temperature = 0.0;
float humidity = 0.0;
float rain_volume = 0.0;
int uv_index = 0;
int lux = 0;
int air_pressure = 0;
float wind_speed = 0.0;
String rain = "no";


// Interrupt function to record swing timestamps
#define MAX_SWINGS 1000     // Maximum number of swings to store (adjust based on memory)

unsigned long swingTimestamps[MAX_SWINGS]; // Circular buffer to store timestamps
int swingStart = 0; // Index of the oldest timestamp
int swingEnd = 0;   // Index of the next available slot

// Interrupt function to record swing timestamps
void countSwing() {
    unsigned long currentMillis = millis();

    // Add the current timestamp to the circular buffer
    swingTimestamps[swingEnd] = currentMillis;
    swingEnd = (swingEnd + 1) % MAX_SWINGS;

    // If the buffer is full, overwrite the oldest timestamp
    if (swingEnd == swingStart) {
        swingStart = (swingStart + 1) % MAX_SWINGS;
    }
}

// Function to calculate rain volume over the last 6 hours
float rainVolume() {
    unsigned long currentMillis = millis();
    int validSwings = 0;

    // Count timestamps within the last 6 hours
    for (int i = swingStart; i != swingEnd; i = (i + 1) % MAX_SWINGS) {
        if (currentMillis - swingTimestamps[i] <= six_hours) {
            validSwings++;
        } else {
            // If a timestamp is older than 6 hours, move the start index forward
            swingStart = (swingStart + 1) % MAX_SWINGS;
        }
    }

    // Calculate rain volume based on the valid swings
    float volume = (validSwings * mili_liter_per_swing) / 1000.0; // Convert to liters
    return volume/0.01; //return volume per meter square
}

// Function to calculate rotary and wind speed
void countHole() {
  holeCount+=1;
}

float calculateRotarySpeed() {
  unsigned long currentMillis = millis();
  
  // Calculate RPM every INTERVAL
  if (currentMillis - lastMeasurementTime_1 >= INTERVAL) {
    lastMeasurementTime_1 = currentMillis;

    // Calculate omega (angular velocity)
    float angle = holeCount * 18 * (PI/180);
    omega = (angle / (INTERVAL / 1000.0));
    wind_speed = omega * 0.065;

    // Reset hole count
    holeCount = 0;
    }
  return wind_speed;
 }

// Function to convert analog data to uv index
int uv_convert(int uv_data) {
  return (uv_data * 5 / 4095) * 10;
  }

// Function to update sensor readings
void updateSensorData() {
  uv_index = uv_convert(analogRead(25));
  
  lux = lightMeter.readLightLevel();

  if (bmp.takeForcedMeasurement()) {
    air_pressure = bmp.readPressure();
  }

  sensors_event_t sht_humidity, sht_temp;
  sht4.getEvent(&sht_humidity, &sht_temp);
  humidity = sht_humidity.relative_humidity;
  temperature = sht_temp.temperature;

  // Check for failed readings
  if (isnan(humidity) || isnan(temperature)) {
    humidity = temperature = 0.0; // Reset to default values if failure
  }
}

// Function to serve the main webpage
void handleRoot() {
  // Embed sensor data directly in the HTML
  String html = "<!DOCTYPE html>"
                "<html>"
                "<head><title>Weather Station</title></head>"
                "<body>"
                "<h1>Weather Station Data</h1>"
                "<p>Temperature: " + String(temperature) + " &deg;C</p>"
                "<p>Humidity: " + String(humidity) + " %</p>"
                "<p>Rain volume: " + String(rain_volume) + "</p>"
                "<p>UV index: " + String(uv_index) + "</p>"
                "<p>LUX: " + String(lux) + "</p>"
                "<p>air pressure: " + String(air_pressure) + "</p>"
                "<p>wind_speed: " + String(wind_speed) + "</p>"
                "</body>"
                "</html>";

  server.send(200, "text/html", html);
}

// Function to serve sensor data as JSON (For API use)
void handleSensorData() {
  String json = "{";
  json += "\"temperature\":" + String(temperature) + ",";
  json += "\"humidity\":" + String(humidity) + ",";
  json += "\"rain\":\"" + rain + "\",";
  json += "\"rain_volume\":" + String(rain_volume) + ",";
  json += "\"uv_index\":" + String(uv_index) + ",";
  json += "\"lux\":" + String(lux) + ",";
  json += "\"air_pressure\":" + String(air_pressure) + ",";
  json += "\"wind_speed\":" + String(wind_speed);
  json += "}";
  server.sendHeader("Access-Control-Allow-Origin", "*"); // Allow all origins
  server.send(200, "application/json", json);
}

//Function for tracking the sun position
void solar_track() {
  if (lux > 100) { // Only track the sun when light intensity is greater than 100
    if (!servo.attached()) { // Attach the servo if not already attached
      servo.attach(19);
      Serial.println("Servo attached for solar tracking.");
    }
    // Get the LDR sensor values
    int ldr1 = analogRead(LDR1) * 1.13;
    int ldr2 = analogRead(LDR2);

    // Get the difference of these values
    int difference = abs(ldr1 - ldr2);

    // Check these values using a condition
    if (difference > error) {
      if (ldr1 > ldr2 && Spoint >= 40) {
        Spoint = --Spoint;
      }
      if (ldr1 < ldr2 && Spoint <= 140) {
        Spoint = ++Spoint;
      }
      servo.write(Spoint); // Write the position to the servo
    }

    delay(80); // Small delay for smooth movement
  } else {
    if (servo.attached()) { // Detach the servo when light intensity is low
      servo.detach();
      Serial.println("Servo detached due to low light intensity.");
    }
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println(F("DHTxx test!"));
  
  //UV sensor
  pinMode(25, INPUT);
  
  // Setup the servo
  servo.attach(19);
  servo.write(Spoint);
  delay(1000);

  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("WiFi connected.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  //Pressure sensor setup
  bmp.begin(0x76);
  if (!bmp.begin(0x76)) {
    Serial.println("Error: Could not initialize BMP280!");
    }
  bmp.setSampling(Adafruit_BMP280::MODE_FORCED,     /* Operating Mode. */
                  Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                  Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                  Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                  Adafruit_BMP280::STANDBY_MS_2000); /* Standby time. */

  // Anemometer setup
  pinMode(ENCODER_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN), countHole, FALLING);

  // Rain Gauge setup
  pinMode(RAIN_GAUGE_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(RAIN_GAUGE_PIN), countSwing, FALLING);

  // Initialize LUX sensor
  Wire.begin();
  lightMeter.begin();

  // Initialize temperature and humidity sensor
  sht4.begin();
  sht4.setPrecision(SHT4X_HIGH_PRECISION);
  sht4.setHeater(SHT4X_NO_HEATER);

  // Define server routes
  server.on("/", handleRoot);           // Serve the main webpage
  server.on("/sensor", handleSensorData); // Optional JSON endpoint for sensor data
  server.begin();

  for (byte address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    if (Wire.endTransmission() == 0) {
      Serial.print("Found I2C device at address 0x");
      Serial.println(address, HEX);
      delay(10);
    }
  }
}

void loop() {
  // Handle client requests
  server.handleClient();

  // Calculating the current wind speed
  calculateRotarySpeed();

  // Calculate rain volume
  rain_volume = rainVolume();

  // Update sensor data every 2 seconds
  unsigned long currentMillis = millis();
  if (currentMillis - lastSensorReadTime >= sensorReadInterval) {
    lastSensorReadTime = currentMillis;
    updateSensorData();
  }
  
  //Tracking the sun position
  solar_track();
}
