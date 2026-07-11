/*
  =====================================================================
  Battery Thermal Management System (BTMS)
  Platform     : ESP32 Dev Board
  Application  : 2.4 kW Electric Two-Wheeler — 48V/50Ah Li-ion Pack
  Author       : <Your Name>
  College      : <Your College Name>
  =====================================================================

  FUNCTION:
  - Reads 4x DS18B20 temperature sensors (one per battery module)
  - Reads pack current (ACS712) and pack voltage (voltage divider module)
  - Reads ambient humidity/temp (DHT22) and gas/smoke level (MQ-2)
  - Displays live status on 0.96" OLED
  - Drives a cooling fan relay when temp crosses WARNING threshold
  - Trips a battery cutoff relay + buzzer when temp crosses CRITICAL
    threshold or gas sensor detects abnormal venting (thermal runaway cue)
  - Publishes live data to ThingSpeak every 20s for remote monitoring

  LIBRARIES REQUIRED (Arduino IDE > Library Manager):
  - OneWire
  - DallasTemperature
  - Adafruit_SSD1306 + Adafruit_GFX
  - DHT sensor library (Adafruit)
  - WiFi.h (built-in with ESP32 board package)
  - HTTPClient.h (built-in with ESP32 board package)
  =====================================================================
*/

#include <WiFi.h>
#include <HTTPClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>

// ---------------------- USER CONFIG ---------------------------------
const char* WIFI_SSID     = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char* TS_API_KEY    = "YOUR_THINGSPEAK_WRITE_API_KEY";
const char* TS_SERVER     = "http://api.thingspeak.com/update";

// ---------------------- PIN MAP --------------------------------------
#define ONE_WIRE_BUS      4     // DS18B20 data line (needs 4.7k pull-up to 3.3V)
#define CURRENT_SENSOR_PIN 34   // ACS712 analog output
#define VOLTAGE_SENSOR_PIN 35   // Voltage divider module output
#define DHT_PIN            14
#define DHT_TYPE            DHT22
#define GAS_SENSOR_PIN      32  // MQ-2 analog output
#define FAN_RELAY_PIN       26
#define CUTOFF_RELAY_PIN    27
#define BUZZER_PIN          25
#define OLED_SDA             21
#define OLED_SCL             22
#define SCREEN_WIDTH        128
#define SCREEN_HEIGHT        64

// ---------------------- THRESHOLDS ------------------------------------
const float TEMP_WARNING_C   = 45.0;   // Turn cooling fan ON
const float TEMP_SAFE_C      = 38.0;   // Turn cooling fan OFF (hysteresis)
const float TEMP_CRITICAL_C  = 60.0;   // Trip battery cutoff relay + alarm
const int   GAS_CRITICAL_ADC = 2800;   // Tune after calibrating MQ-2 in clean air

// ---------------------- SENSOR OBJECTS ---------------------------------
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature tempSensors(&oneWire);
DHT dht(DHT_PIN, DHT_TYPE);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ACS712 30A module: sensitivity ~66mV/A, zero-current offset ~ VCC/2
const float ACS712_SENSITIVITY = 0.066; // V/A for 30A module
const float ACS712_ZERO_V      = 1.65;  // measured with no load, calibrate on your board
const float ADC_REF_V          = 3.3;
const int   ADC_RES             = 4095;

// Voltage divider module: adjust RATIO to match your resistor pair
const float VOLTAGE_DIVIDER_RATIO = 7.5; // e.g. 30k/(30k+195k)-style module scaled for 0-25V range

unsigned long lastCloudPush = 0;
const unsigned long CLOUD_INTERVAL_MS = 20000; // 20 seconds
bool fanState = false;
bool cutoffTripped = false;

void setup() {
  Serial.begin(115200);

  pinMode(FAN_RELAY_PIN, OUTPUT);
  pinMode(CUTOFF_RELAY_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(FAN_RELAY_PIN, LOW);
  digitalWrite(CUTOFF_RELAY_PIN, LOW);   // LOW = pack connected (adjust to your relay logic)
  digitalWrite(BUZZER_PIN, LOW);

  tempSensors.begin();
  dht.begin();

  Wire.begin(OLED_SDA, OLED_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("OLED init failed"));
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("BTMS Booting...");
  display.display();

  connectWiFi();
}

void loop() {
  // ---------- 1. Read all sensors ----------
  tempSensors.requestTemperatures();
  float t1 = tempSensors.getTempCByIndex(0);
  float t2 = tempSensors.getTempCByIndex(1);
  float t3 = tempSensors.getTempCByIndex(2);
  float t4 = tempSensors.getTempCByIndex(3);
  float maxTemp = maxOf4(t1, t2, t3, t4);

  float packVoltage = readPackVoltage();
  float packCurrent = readPackCurrent();
  float packPowerW  = packVoltage * packCurrent;

  float ambientTemp = dht.readTemperature();
  float ambientHum   = dht.readHumidity();
  int   gasRaw        = analogRead(GAS_SENSOR_PIN);

  // ---------- 2. Thermal control logic (with hysteresis) ----------
  if (!cutoffTripped) {
    if (maxTemp >= TEMP_WARNING_C && !fanState) {
      digitalWrite(FAN_RELAY_PIN, HIGH);
      fanState = true;
    } else if (maxTemp <= TEMP_SAFE_C && fanState) {
      digitalWrite(FAN_RELAY_PIN, LOW);
      fanState = false;
    }

    // ---------- 3. Safety cutoff (critical temp OR gas/venting detected) ----------
    if (maxTemp >= TEMP_CRITICAL_C || gasRaw >= GAS_CRITICAL_ADC) {
      digitalWrite(CUTOFF_RELAY_PIN, HIGH);  // open contactor / isolate pack
      digitalWrite(BUZZER_PIN, HIGH);
      cutoffTripped = true;
      Serial.println("!! CRITICAL: Battery isolated. Manual reset required. !!");
    }
  }

  // ---------- 4. OLED display ----------
  updateDisplay(t1, t2, t3, t4, packVoltage, packCurrent, ambientTemp, ambientHum, fanState, cutoffTripped);

  // ---------- 5. Serial log (also usable for local CSV logging via Serial monitor) ----------
  Serial.printf("T1:%.1f T2:%.1f T3:%.1f T4:%.1f | V:%.2f I:%.2f P:%.1fW | Fan:%d Cutoff:%d Gas:%d\n",
                t1, t2, t3, t4, packVoltage, packCurrent, packPowerW, fanState, cutoffTripped, gasRaw);

  // ---------- 6. Push to cloud dashboard periodically ----------
  if (millis() - lastCloudPush > CLOUD_INTERVAL_MS) {
    pushToThingSpeak(maxTemp, packVoltage, packCurrent, ambientTemp, gasRaw, fanState, cutoffTripped);
    lastCloudPush = millis();
  }

  delay(2000);
}

// ---------------------- HELPER FUNCTIONS ------------------------------

float maxOf4(float a, float b, float c, float d) {
  float m = a;
  if (b > m) m = b;
  if (c > m) m = c;
  if (d > m) m = d;
  return m;
}

float readPackVoltage() {
  int raw = analogRead(VOLTAGE_SENSOR_PIN);
  float vAtPin = (raw / (float)ADC_RES) * ADC_REF_V;
  return vAtPin * VOLTAGE_DIVIDER_RATIO;
}

float readPackCurrent() {
  int raw = analogRead(CURRENT_SENSOR_PIN);
  float vAtPin = (raw / (float)ADC_RES) * ADC_REF_V;
  return (vAtPin - ACS712_ZERO_V) / ACS712_SENSITIVITY;
}

void updateDisplay(float t1, float t2, float t3, float t4, float v, float i,
                    float amb, float hum, bool fan, bool cutoff) {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.printf("T1:%.1f T2:%.1f\n", t1, t2);
  display.printf("T3:%.1f T4:%.1f\n", t3, t4);
  display.printf("V:%.1fV I:%.1fA\n", v, i);
  display.printf("Amb:%.1fC H:%.0f%%\n", amb, hum);
  display.printf("Fan:%s\n", fan ? "ON" : "OFF");
  if (cutoff) display.println("!! PACK ISOLATED !!");
  display.display();
}

void connectWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  Serial.println(WiFi.status() == WL_CONNECTED ? "\nConnected!" : "\nWiFi failed - running offline.");
}

void pushToThingSpeak(float maxTemp, float v, float i, float amb, int gas, bool fan, bool cutoff) {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  String url = String(TS_SERVER) + "?api_key=" + TS_API_KEY +
               "&field1=" + String(maxTemp) +
               "&field2=" + String(v) +
               "&field3=" + String(i) +
               "&field4=" + String(amb) +
               "&field5=" + String(gas) +
               "&field6=" + String(fan) +
               "&field7=" + String(cutoff);
  http.begin(url);
  int code = http.GET();
  Serial.printf("ThingSpeak push status: %d\n", code);
  http.end();
}
