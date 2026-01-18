#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <FastLED.h>

// WiFi Configuration
const char* WIFI_SSID = "Raihan's IOT";
const char* WIFI_PASS = "Raihanratul";

// Server
ESP8266WebServer server(80);

// LED Configuration
#define NUM_STRIPS 5
#define MAX_LEDS_PER_STRIP 150
#define MIC_PIN A0

struct StripConfig {
  uint8_t pin;
  uint16_t ledCount;
  bool enabled;
  CRGB* leds;
};

StripConfig strips[NUM_STRIPS] = {
  {5, 0, false, nullptr},   // D1
  {4, 0, false, nullptr},   // D2
  {0, 0, false, nullptr},   // D3
  {2, 0, false, nullptr},   // D4
  {14, 0, false, nullptr}   // D5
};

// Animation Variables
uint8_t currentMode = 0;
uint8_t brightness = 100;
CRGB currentColor = CRGB::White;
bool manualMode = false;

// Music Reactive
int audioValue = 0;
int audioMax = 0;
int sensitivity = 50;

// Animation
unsigned long lastUpdate = 0;
uint16_t hue = 0;

void setup() {
  Serial.begin(115200);
  
  // Initialize LittleFS
  if (!LittleFS.begin()) {
    Serial.println("LittleFS Mount Failed");
    return;
  }
  
  // Load Configuration
  loadConfig();
  
  // Initialize WiFi
  setupWiFi();
  
  // Initialize Web Server
  setupWebServer();
  
  // Initialize LED Strips
  initLEDStrips();
  
  Serial.println("Setup Complete!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  server.handleClient();
  
  if (!manualMode) {
    updateAnimation();
  }
  
  // Music reactive sampling
  if (currentMode == 4) { // Music mode
    updateMusicReactive();
  }
  
  FastLED.show();
}

// ==================== WEB SERVER ====================
void setupWebServer() {
  // Serve static files
  server.on("/", HTTP_GET, []() {
    File file = LittleFS.open("/index.html", "r");
    Serial.print(file);
    server.streamFile(file, "text/html");
    Serial.println("hit index.html");
    file.close();
  });
  
  server.on("/style.css", HTTP_GET, []() {
    File file = LittleFS.open("/style.css", "r");
    server.streamFile(file, "text/css");
    Serial.println("hit style");
    file.close();
  });
  
  server.on("/script.js", HTTP_GET, []() {
    File file = LittleFS.open("/script.js", "r");
    server.streamFile(file, "text/javascript");
    Serial.printoln("hit js");
    file.close();
  });
  
  // API Endpoints
  server.on("/api/config", HTTP_GET, handleGetConfig);
  server.on("/api/config", HTTP_POST, handleSaveConfig);
  server.on("/api/mode", HTTP_POST, handleSetMode);
  server.on("/api/brightness", HTTP_POST, handleSetBrightness);
  server.on("/api/color", HTTP_POST, handleSetColor);
  server.on("/api/led", HTTP_POST, handleSetLED);
  server.on("/api/status", HTTP_GET, handleGetStatus);
  
  server.begin();
}

void setupWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  Serial.print("Connecting to WiFi");

  unsigned long startAttempt = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (millis() - startAttempt > 20000) {
      Serial.println("\nWiFi Failed");
      return;
    }
  }

  Serial.println("\nWiFi Connected");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}


// ==================== CONFIGURATION ====================
void loadConfig() {
  File configFile = LittleFS.open("/config.json", "r");
  if (!configFile) {
    Serial.println("No config file found, using defaults");
    return;
  }
  
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, configFile);
  configFile.close();
  
  if (!error) {
    for (int i = 0; i < NUM_STRIPS; i++) {
      strips[i].ledCount = doc["strips"][i]["ledCount"] | 0;
      strips[i].enabled = doc["strips"][i]["enabled"] | false;
    }
    currentMode = doc["mode"] | 0;
    brightness = doc["brightness"] | 100;
    currentColor = CRGB(doc["color"]["r"] | 255, 
                        doc["color"]["g"] | 255, 
                        doc["color"]["b"] | 255);
  }
}

void saveConfig() {
  DynamicJsonDocument doc(1024);
  
  JsonArray stripsArray = doc.createNestedArray("strips");
  for (int i = 0; i < NUM_STRIPS; i++) {
    JsonObject stripObj = stripsArray.createNestedObject();
    stripObj["pin"] = strips[i].pin;
    stripObj["ledCount"] = strips[i].ledCount;
    stripObj["enabled"] = strips[i].enabled;
  }
  
  doc["mode"] = currentMode;
  doc["brightness"] = brightness;
  doc["color"]["r"] = currentColor.r;
  doc["color"]["g"] = currentColor.g;
  doc["color"]["b"] = currentColor.b;
  
  File configFile = LittleFS.open("/config.json", "w");
  serializeJson(doc, configFile);
  configFile.close();
}

// ==================== LED STRIPS ====================
// void initLEDStrips() {
//   int totalLeds = 0;
  
//   for (int i = 0; i < NUM_STRIPS; i++) {
//     if (strips[i].enabled && strips[i].ledCount > 0) {
//       strips[i].leds = new CRGB[strips[i].ledCount];
//       // FastLED.addLeds<WS2812B, GPIO_PIN, GRB>(strips[i].leds, strips[i].ledCount);
//       FastLED.addLeds<WS2812B>(strips[i].leds, strips[i].ledCount).setPin(strips[i].pin);

//       totalLeds += strips[i].ledCount;
//     }
//   }
  
//   FastLED.setBrightness(brightness);
//   Serial.print("Total LEDs: ");
//   Serial.println(totalLeds);
// }
void initLEDStrips() {
  if (strips[0].enabled)
    FastLED.addLeds<WS2812B, 5, GRB>(strips[0].leds, strips[0].ledCount);   // D1

  if (strips[1].enabled)
    FastLED.addLeds<WS2812B, 4, GRB>(strips[1].leds, strips[1].ledCount);   // D2

  if (strips[2].enabled)
    FastLED.addLeds<WS2812B, 0, GRB>(strips[2].leds, strips[2].ledCount);   // D3

  if (strips[3].enabled)
    FastLED.addLeds<WS2812B, 2, GRB>(strips[3].leds, strips[3].ledCount);   // D4

  if (strips[4].enabled)
    FastLED.addLeds<WS2812B, 14, GRB>(strips[4].leds, strips[4].ledCount);  // D5

  FastLED.setBrightness(brightness);
}
// ==================== ANIMATIONS ====================
void updateAnimation() {
  unsigned long currentMillis = millis();
  
  if (currentMillis - lastUpdate < 30) return;
  lastUpdate = currentMillis;
  
  switch (currentMode) {
    case 0: // Static Color
      setAllColor(currentColor);
      break;
      
    case 1: // Rainbow
      hue++;
      for (int i = 0; i < NUM_STRIPS; i++) {
        if (strips[i].enabled) {
          fill_rainbow(strips[i].leds, strips[i].ledCount, hue, 7);
        }
      }
      break;
      
    case 2: // Breathing
      {
        uint8_t breath = (exp(sin(millis()/2000.0*PI)) - 0.36787944)*108.0;
        FastLED.setBrightness(breath);
        setAllColor(currentColor);
      }
      break;
      
    case 3: // Chase
      {
        static uint16_t offset = 0;
        offset++;
        for (int i = 0; i < NUM_STRIPS; i++) {
          if (strips[i].enabled) {
            for (int j = 0; j < strips[i].ledCount; j++) {
              strips[i].leds[j] = CHSV((offset + j) % 255, 255, 255);
            }
          }
        }
      }
      break;
      
    case 4: // Music Reactive
      // Handled separately
      break;
  }
}

void updateMusicReactive() {
  audioValue = analogRead(MIC_PIN);
  audioMax = max(audioMax, audioValue);
  
  for (int i = 0; i < NUM_STRIPS; i++) {
    if (strips[i].enabled) {
      int numLit = map(audioValue, 0, audioMax, 0, strips[i].ledCount);
      for (int j = 0; j < strips[i].ledCount; j++) {
        if (j < numLit) {
          strips[i].leds[j] = CHSV(map(j, 0, strips[i].ledCount, 0, 255), 255, 255);
        } else {
          strips[i].leds[j] = CRGB::Black;
        }
      }
    }
  }
  
  // Slowly reset max
  if (millis() % 1000 == 0) {
    audioMax = audioMax * 0.9;
  }
}

void setAllColor(CRGB color) {
  for (int i = 0; i < NUM_STRIPS; i++) {
    if (strips[i].enabled) {
      fill_solid(strips[i].leds, strips[i].ledCount, color);
    }
  }
}

// ==================== API HANDLERS ====================
void handleGetConfig() {
  DynamicJsonDocument doc(2048);
  
  JsonArray stripsArray = doc.createNestedArray("strips");
  for (int i = 0; i < NUM_STRIPS; i++) {
    JsonObject strip = stripsArray.createNestedObject();
    strip["pin"] = strips[i].pin;
    strip["ledCount"] = strips[i].ledCount;
    strip["enabled"] = strips[i].enabled;
  }
  
  doc["totalLeds"] = getTotalLEDs();
  doc["mode"] = currentMode;
  doc["brightness"] = brightness;
  doc["manualMode"] = manualMode;
  
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleSaveConfig() {
  String body = server.arg("plain");
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, body);
  
  for (int i = 0; i < NUM_STRIPS; i++) {
    strips[i].ledCount = doc["strips"][i]["ledCount"];
    strips[i].enabled = doc["strips"][i]["enabled"];
  }
  
  // Reinitialize LEDs with new config
  FastLED.clear();
  initLEDStrips();
  saveConfig();
  
  server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void handleSetMode() {
  currentMode = server.arg("mode").toInt();
  manualMode = (currentMode == 255); // Special mode for manual control
  saveConfig();
  server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void handleSetBrightness() {
  brightness = server.arg("brightness").toInt();
  FastLED.setBrightness(brightness);
  saveConfig();
  server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void handleSetColor() {
  uint8_t r = server.arg("r").toInt();
  uint8_t g = server.arg("g").toInt();
  uint8_t b = server.arg("b").toInt();
  currentColor = CRGB(r, g, b);
  saveConfig();
  server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void handleSetLED() {
  if (!manualMode) {
    server.send(400, "application/json", "{\"error\":\"Manual mode not active\"}");
    return;
  }
  
  int stripIndex = server.arg("strip").toInt();
  int ledIndex = server.arg("led").toInt();
  uint8_t r = server.arg("r").toInt();
  uint8_t g = server.arg("g").toInt();
  uint8_t b = server.arg("b").toInt();
  
  if (stripIndex >= 0 && stripIndex < NUM_STRIPS &&
      ledIndex >= 0 && ledIndex < strips[stripIndex].ledCount &&
      strips[stripIndex].enabled) {
    strips[stripIndex].leds[ledIndex] = CRGB(r, g, b);
  }
  
  server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void handleGetStatus() {
  DynamicJsonDocument doc(512);
  doc["activeStrips"] = getActiveStripCount();
  doc["totalLeds"] = getTotalLEDs();
  doc["freeHeap"] = ESP.getFreeHeap();
  doc["uptime"] = millis() / 1000;
  
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

int getTotalLEDs() {
  int total = 0;
  for (int i = 0; i < NUM_STRIPS; i++) {
    if (strips[i].enabled) {
      total += strips[i].ledCount;
    }
  }
  return total;
}

int getActiveStripCount() {
  int count = 0;
  for (int i = 0; i < NUM_STRIPS; i++) {
    if (strips[i].enabled) count++;
  }
  return count;
}