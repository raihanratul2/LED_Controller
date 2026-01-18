// #include <Adafruit_NeoPixel.h>

// #define LED_PIN     6
// #define NUM_LEDS    300

// Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// void setup() {
//   strip.begin();
//   strip.show(); // Initially turn off all LEDs
// }

// // Function to set a specific LED's color (no show here)
// void setLEDColor(int index, uint8_t r, uint8_t g, uint8_t b) {
//   if (index < NUM_LEDS) {
//     strip.setPixelColor(index, strip.Color(r, g, b));
//   }
// }

// // Set all LEDs and update once
// void setAllLEDs(uint8_t r, uint8_t g, uint8_t b) {
//   for (int i = 0; i < NUM_LEDS; i++) {
//     setLEDColor(i, r, g, b);  // Just set, don't show
//   }
//   strip.show();  // Show once for all
// }

// void loop() {
//   setAllLEDs(255, 0, 0);     // Red
//   delay(2000);

//   setAllLEDs(0, 255, 0);     // Green
//   delay(2000);

//   setAllLEDs(0, 0, 255);     // Blue
//   delay(2000);

//   setAllLEDs(255, 255, 255); // White
//   delay(2000);

//   strip.clear();
//   strip.show();
// }
#include <Adafruit_NeoPixel.h>

#define LED_PIN     6
#define NUM_LEDS    300

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  strip.begin();
  strip.show();
}

// Function to set all LEDs to a color
void setAllLEDs(uint8_t r, uint8_t g, uint8_t b) {
  for (int i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, r, g, b);
  }
  strip.show();
}

// Smooth color transition between two colors
void fadeColor(uint8_t r1, uint8_t g1, uint8_t b1, uint8_t r2, uint8_t g2, uint8_t b2, int steps, int delayTime) {
  for (int i = 0; i <= steps; i++) {
    uint8_t r = r1 + (r2 - r1) * i / steps;
    uint8_t g = g1 + (g2 - g1) * i / steps;
    uint8_t b = b1 + (b2 - b1) * i / steps;
    setAllLEDs(r, g, b);
    delay(delayTime);
  }
}

void loop() {
  fadeColor(255, 0, 0,   0, 0, 255,   100, 20);  // Red → Blue
  fadeColor(0, 0, 255,   0, 255, 0,   100, 20);  // Blue → Green
  fadeColor(0, 255, 0,   255, 0, 0,   100, 20);  // Green → Red
}
