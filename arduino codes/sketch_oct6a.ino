/*
 * PedalGuard - ESP8266 Anti-Theft System
 * * Description:
 * This code runs on a NodeMCU 1.0 (ESP8266) board and acts as a bike or vehicle
 * anti-theft device. It now uses an interrupt to monitor the vibration sensor
 * (SW-420) for more reliable detection.
 * The accelerometer (ADXL345) code is currently disabled.
 * The system's state (ARMED/DISARMED) is controlled by a remote web application.
 * If motion is detected while armed, it sounds a local buzzer for a set duration,
 * sends an alert, and then automatically re-arms itself.
 * * Hardware Required:
 * - NodeMCU 1.0 (ESP8266)
 * - SW-420 Vibration Sensor Module
 * - NEO-6M GPS Module (Serial)
 * - Piezo Buzzer
 * - 2x LEDs (Red, Green)
 * - Resistors for LEDs (e.g., 220 Ohm)
 * - Breadboard and jumper wires
 * * Web Server API Endpoints:
 * - GET /api/getstatus?deviceId=123 -> Responds with {"status": "ARMED"} or {"status": "DISARMED"}
 * - POST /api/alert -> Receives a JSON payload with alert details.
 *
 * Version: 2.0 - Accurate Timestamp Handling
 * This is the final Arduino code for ESP8266
 */

// --- LIBRARIES ---
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <SoftwareSerial.h>
#include <TinyGPS++.h>
// #include <Wire.h> // ADXL345 Library
// #include <Adafruit_ADXL345_U.h> // ADXL345 Library
// #include <Adafruit_Sensor.h> // ADXL345 Library

// --- CONFIGURATION ---
const char* ssid = "OnePlus 12";
const char* password = "dkbhotspot123";

// Web App URLs - Updated with your specific IP Address
const char* getStatusUrl = "http://10.186.70.155:3000/api/getstatus?deviceId=123";
const char* sendAlertUrl = "http://10.186.70.155:3000/api/alert";
const char* deviceId = "123";

// --- PIN DEFINITIONS (NodeMCU Pinout) ---
#define VIBRATION_PIN   D5  // SW-420 sensor Digital Out (DO) pin
#define BUZZER_PIN      D8  // Buzzer positive (+) pin
#define GPS_RX_PIN      D7  // SoftwareSerial RX: Connects to NEO-6M TX pin
#define GPS_TX_PIN      D6  // SoftwareSerial TX: Connects to NEO-6M RX pin
#define RED_LED_PIN     D3  // Red LED anode (+) via 220Ω resistor
#define GREEN_LED_PIN   D4  // Green LED anode (+) via 220Ω resistor

// --- SENSOR THRESHOLDS ---
const unsigned long GPS_DATA_MAX_AGE = 2000; // Only use GPS data less than 2 seconds old
const unsigned long ALARM_DURATION = 10000; // Alarm runs for 10 seconds before re-arming

// --- GLOBAL OBJECTS & VARIABLES ---
TinyGPSPlus gps;
SoftwareSerial ss(GPS_RX_PIN, GPS_TX_PIN);

enum SystemState { DISARMED, ARMED, TRIGGERED };
SystemState currentState = DISARMED;

unsigned long lastStatusCheck = 0;
const long statusCheckInterval = 5000;

unsigned long lastAlarmActionTime = 0;
// Variables for the alarm pattern
int alarmPatternStep = 0;
const long alarmPatternIntervals[] = {150, 150, 150, 600}; // on, off, on, off(long pause)
const int alarmPatternNumSteps = 4;

// --- INTERRUPT & GRACE PERIOD VARIABLES ---
volatile bool vibrationFlag = false; // Flag set by ISR to signal a vibration
unsigned long timeArmed = 0; // Timestamp for arming grace period
unsigned long timeTriggered = 0; // Timestamp for alarm duration

// --- INTERRUPT SERVICE ROUTINE (ISR) ---
// This function runs in the background whenever the vibration sensor pin goes LOW.
ICACHE_RAM_ATTR void handleVibration() {
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  if (interrupt_time - last_interrupt_time > 5000) { // 5000ms (5 second) debounce window
    vibrationFlag = true;
  }
  last_interrupt_time = interrupt_time;
}

// ===============================================================
// SETUP: Runs once on boot
// ===============================================================
void setup() {
  Serial.begin(115200);
  ss.begin(9600);

  pinMode(VIBRATION_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  
  attachInterrupt(digitalPinToInterrupt(VIBRATION_PIN), handleVibration, FALLING);
  
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(RED_LED_PIN, LOW);
  digitalWrite(GREEN_LED_PIN, HIGH);

  Serial.println("\n--- PedalGuard Initializing (ADXL345 Disabled) ---");

  connectToWiFi();

  Serial.println("ADXL345 check skipped.");
  Serial.println("Setup complete. Awaiting commands from server...");
}

// ===============================================================
// LOOP: Runs continuously
// ===============================================================
void loop() {
  // Process GPS data on every single loop iteration for reliability
  processGps();

  // Check for server status periodically
  unsigned long currentTime = millis();
  if (currentTime - lastStatusCheck > statusCheckInterval) {
    checkSystemStatus();
    lastStatusCheck = currentTime;
  }

  // Handle the main state machine logic
  switch (currentState) {
    case ARMED:
      monitorSensors();
      break;
    case TRIGGERED:
      runAlarmSequence();
      // Check if the alarm duration has passed to re-arm
      if (millis() - timeTriggered > ALARM_DURATION) {
        Serial.println("Alarm sequence finished. Re-arming system.");
        currentState = ARMED;
        timeArmed = millis();     // Reset the arming grace period
        vibrationFlag = false;    // Clear any pending flags
        digitalWrite(BUZZER_PIN, LOW); // Turn off the buzzer
        digitalWrite(RED_LED_PIN, HIGH); // Set LED to solid red for ARMED state
      }
      break;
    case DISARMED:
      // Do nothing
      break;
  }
}

// ===============================================================
// --- CUSTOM FUNCTIONS ---
// ===============================================================

/**
 * @brief Reads all available characters from the GPS serial port and feeds them to the TinyGPS++ library.
 * This should be called as frequently as possible from the main loop.
 */
void processGps() {
  while (ss.available() > 0) {
    gps.encode(ss.read());
  }
}

void connectToWiFi() {
  Serial.print("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  
  int retries = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (retries++ > 20) {
        Serial.println("\nFailed to connect to WiFi. Please check credentials. Restarting...");
        ESP.restart();
    }
  }
  
  Serial.println("\nWiFi connected! IP: " + WiFi.localIP().toString());
}

void checkSystemStatus() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected. Reconnecting...");
    connectToWiFi();
    return;
  }

  HTTPClient http;
  WiFiClient client;

  Serial.print("Checking system status from server... ");
  http.begin(client, getStatusUrl);

  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    Serial.println("Received: " + payload);

    StaticJsonDocument<128> doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (error) {
      Serial.print("deserializeJson() failed: ");
      Serial.println(error.c_str());
    } else {
      const char* status = doc["status"];

      if (strcmp(status, "ARMED") == 0) {
        if (currentState == DISARMED) {
          Serial.println("System state changed to: ARMED");
          currentState = ARMED;
          timeArmed = millis();     // Set the timestamp for the grace period
          vibrationFlag = false;    // Clear any stray flags that occurred during state change
          digitalWrite(RED_LED_PIN, HIGH);
          digitalWrite(GREEN_LED_PIN, LOW);
          digitalWrite(BUZZER_PIN, LOW);
        }
      } else {
        if (currentState != DISARMED) {
          Serial.println("System state changed to: DISARMED");
          currentState = DISARMED;
          digitalWrite(RED_LED_PIN, LOW);
          digitalWrite(GREEN_LED_PIN, HIGH);
          digitalWrite(BUZZER_PIN, LOW);
          alarmPatternStep = 0; // Reset the alarm pattern when disarmed
        }
      }
    }
  } else {
    Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();
}

void monitorSensors() {
  if (millis() < timeArmed + 1000) {
    vibrationFlag = false; 
    return;
  }

  if (vibrationFlag) {
    vibrationFlag = false;
    Serial.println("!!! VIBRATION DETECTED !!!");
    triggerAlarm("Vibration");
  }
}

void triggerAlarm(const char* triggerSource) {
  if (currentState != ARMED) return;
  
  Serial.println("--- ALARM TRIGGERED ---");
  currentState = TRIGGERED;
  timeTriggered = millis();
  
  alarmPatternStep = 0;
  lastAlarmActionTime = millis();
  digitalWrite(BUZZER_PIN, HIGH);
  digitalWrite(RED_LED_PIN, HIGH);

  sendAlertToWebApp(triggerSource);
}

void runAlarmSequence() {
  if (millis() - lastAlarmActionTime > alarmPatternIntervals[alarmPatternStep]) {
    lastAlarmActionTime = millis();
    alarmPatternStep = (alarmPatternStep + 1) % alarmPatternNumSteps;
    bool isAlarmOn = (alarmPatternStep == 0 || alarmPatternStep == 2);
    digitalWrite(BUZZER_PIN, isAlarmOn ? HIGH : LOW);
    digitalWrite(RED_LED_PIN, isAlarmOn ? HIGH : LOW);
  }
}

void sendAlertToWebApp(const char* triggerSource) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Cannot send alert - WiFi disconnected");
    return;
  }

  HTTPClient http;
  WiFiClient client;

  http.begin(client, sendAlertUrl);
  http.addHeader("Content-Type", "application/json");

  StaticJsonDocument<512> doc;
  doc["deviceId"] = deviceId;
  doc["type"] = "TheftAlert";
  doc["trigger"] = triggerSource;
  // doc["timestamp"] = millis(); // <-- REMOVED this line to rely on server timestamp

  if (gps.location.isValid() && gps.location.age() < GPS_DATA_MAX_AGE) {
    JsonObject location = doc.createNestedObject("location");
    location["lat"] = gps.location.lat();
    location["lng"] = gps.location.lng();
    location["speedKmph"] = gps.speed.kmph();
  } else {
    doc["location"] = "No fresh GPS data";
  }
  
  String requestBody;
  serializeJson(doc, requestBody);

  Serial.println("Sending alert to server: " + requestBody);

  int httpCode = http.POST(requestBody);

  if (httpCode > 0) {
    String response = http.getString();
    Serial.println("Server response: " + response);
  } else {
    Serial.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();
}

