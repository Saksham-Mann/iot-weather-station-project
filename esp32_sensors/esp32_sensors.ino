#include <Wire.h>
#include <Adafruit_LTR390.h>
#include <Adafruit_BMP085.h>
#include <DHT.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <time.h>

// Provide the token generation process info
#include "addons/TokenHelper.h"
// Provide the Firestore payload printing info
#include "addons/RTDBHelper.h"

#include "secrets.h"

// The values below are now loaded from secrets.h
// #define WIFI_SSID       "..."
// #define WIFI_PASSWORD   "..."
// #define API_KEY         "..."
// #define FIREBASE_PROJECT_ID "..."
// #define DATABASE_URL "..."

// ============================================================
// --- PIN DEFINITIONS ---
// ============================================================
// DHT11
#define DHTPIN 14       
#define DHTTYPE DHT11 

// BMP180 (I2C Bus 1)
// SDA - P17 (S), SCL - P16 (S)
#define BMP_SDA 17
#define BMP_SCL 16

// LTR390 (I2C Bus 2)
// SDA - P18(S), SCL- P19(S)
#define LTR_SDA 18
#define LTR_SCL 19

// ============================================================
// --- OBJECTS ---
// ============================================================
Adafruit_LTR390 ltr = Adafruit_LTR390();
Adafruit_BMP085 bmp;
DHT dht(DHTPIN, DHTTYPE);

// Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Timing
unsigned long sendDataPrevMillis = 0;
unsigned long firestorePrevMillis = 0;
const unsigned long rtdbInterval = 5000;       // RTDB: every 5 seconds
const unsigned long firestoreInterval = 30000; // Firestore: every 30 seconds
bool signupOK = false;

// ============================================================
// --- WIFI SETUP ---
// ============================================================
void connectWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.print("Connected! IP: ");
  Serial.println(WiFi.localIP());
}

// ============================================================
// --- FIREBASE SETUP ---
// ============================================================
void connectFirebase() {
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  // Anonymous sign-up
  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("Firebase sign-up OK");
    signupOK = true;
  } else {
    Serial.printf("Firebase sign-up FAILED: %s\n", config.signer.signupError.message.c_str());
  }

  config.token_status_callback = tokenStatusCallback;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

// ============================================================
// --- NTP TIME SETUP ---
// ============================================================
void setupTime() {
  configTime(19800, 0, "pool.ntp.org", "time.nist.gov"); // IST = UTC+5:30 = 19800s
  Serial.print("Syncing time");
  while (time(nullptr) < 100000) {
    Serial.print(".");
    delay(500);
  }
  Serial.println(" OK");
}

// Get ISO 8601 timestamp string
String getTimestamp() {
  time_t now = time(nullptr);
  struct tm* t = localtime(&now);
  char buf[30];
  strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", t);
  return String(buf);
}

// ============================================================
// --- SETUP ---
// ============================================================
void setup() {
  Serial.begin(115200);
  Serial.println("ESP32 Multi-Sensor + Firebase (RTDB + Firestore)");

  connectWiFi();
  setupTime();
  connectFirebase();

  // I2C Bus 1 (BMP180)
  Wire.begin(BMP_SDA, BMP_SCL);
  // I2C Bus 2 (LTR390)
  Wire1.begin(LTR_SDA, LTR_SCL);

  // Initialize BMP180
  if (!bmp.begin()) {
    Serial.println("Could not find BMP180 sensor! Check wiring (Pins 17/16).");
  } else {
    Serial.println("Found BMP180 sensor!");
  }

  // Initialize LTR390
  if (!ltr.begin(&Wire1)) {
    Serial.println("Could not find LTR390 sensor! Check wiring (Pins 18/19).");
  } else {
    Serial.println("Found LTR390 sensor!");
    ltr.setMode(LTR390_MODE_UVS);
    ltr.setGain(LTR390_GAIN_3);
    ltr.setResolution(LTR390_RESOLUTION_18BIT);
    ltr.setThresholds(100, 1000);
    ltr.configInterrupt(true, LTR390_MODE_UVS);
  }

  // Initialize DHT11
  dht.begin();
  Serial.println("DHT11 Initialized!");
}

// ============================================================
// --- LOOP ---
// ============================================================
void loop() {
  Serial.println("\n------------------------------------");

  // --- Read Sensors ---
  // LTR390 UV
  float uvIndex = 0;
  if (ltr.newDataAvailable()) {
    uint32_t uvs = ltr.readUVS();
    uvIndex = uvs / 2300.0;
    Serial.print("UV Data: "); Serial.print(uvs);
    Serial.print("  UV Index: "); Serial.println(uvIndex);
  }

  // BMP180
  float pressure    = bmp.readPressure();
  float bmpTemp     = bmp.readTemperature();
  float altitude    = bmp.readAltitude();

  if (pressure > 0) {
    Serial.print("BMP Temp: "); Serial.print(bmpTemp); Serial.print(" °C");
    Serial.print("\tPressure: "); Serial.print(pressure); Serial.print(" Pa");
    Serial.print("\tAltitude: "); Serial.print(altitude); Serial.println(" m");
  } else {
    Serial.println("BMP180 Read Error");
  }

  // DHT11
  float humidity    = dht.readHumidity();
  float temperature = dht.readTemperature();

  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
  } else {
    Serial.print("DHT Humidity: "); Serial.print(humidity); Serial.print(" %");
    Serial.print("\tTemp: "); Serial.print(temperature); Serial.println(" °C");
  }

  // ============================================================
  // --- Send to RTDB (every 5s for real-time dashboard) ---
  // ============================================================
  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > rtdbInterval || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();

    if (!isnan(temperature))
      Firebase.RTDB.setFloat(&fbdo, "sensors/temperature", temperature);
    if (!isnan(humidity))
      Firebase.RTDB.setFloat(&fbdo, "sensors/humidity", humidity);
    if (pressure > 0) {
      Firebase.RTDB.setFloat(&fbdo, "sensors/pressure_hPa", pressure / 100.0);
      Firebase.RTDB.setFloat(&fbdo, "sensors/altitude_m", altitude);
    }
    Firebase.RTDB.setFloat(&fbdo, "sensors/uv_index", uvIndex);
    Firebase.RTDB.setInt(&fbdo, "sensors/timestamp", time(nullptr));
    Serial.println("RTDB updated");
  }

  // ============================================================
  // --- Send to Firestore (every 30s with timestamp) ---
  // ============================================================
  if (Firebase.ready() && signupOK && (millis() - firestorePrevMillis > firestoreInterval || firestorePrevMillis == 0)) {
    firestorePrevMillis = millis();

    String timestamp = getTimestamp();
    String documentPath = "sensor_readings/" + timestamp;

    FirebaseJson content;
    
    // Firestore document fields
    content.set("fields/temperature/doubleValue", (double)temperature);
    content.set("fields/humidity/doubleValue", (double)humidity);
    content.set("fields/pressure_hPa/doubleValue", (double)(pressure / 100.0));
    content.set("fields/altitude_m/doubleValue", (double)altitude);
    content.set("fields/uv_index/doubleValue", (double)uvIndex);
    content.set("fields/timestamp/stringValue", timestamp);

    if (Firebase.Firestore.createDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.raw())) {
      Serial.println("Firestore: Document created at " + documentPath);
    } else {
      Serial.println("Firestore FAILED: " + fbdo.errorReason());
    }
  }

  delay(2000);
}
