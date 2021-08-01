// ColourSorter.ino
//
// This is an improved version of the ColourSorter originally built/demonstrated by 
//
//   Michael Klements' Skittles Colour Sorter (www.the-diy-life.com).
//
// Notable changes:
//
// * Cleaner coding style, including use of #define directives and better code structure.
// * Serial port logging of color measurements and selected color.
// * Six colors/tubes instead of 5 to accommodate M&Ms, which add blue (Skittles don't have blue).
// * Colors are selected based on 3-dimensional distance to measured color coordinates. Greatly 
//   improves accuracy of color detection using same input data. Also facilitates a data-driven
//   implementation vs. the original code-driven decision tree.
// * Jostling is added in an attempt to improve selector and sorter function.

#include <Servo.h>
#include <Wire.h>
#include "Adafruit_TCS34725.h"

#define SELECTOR_LIMIT      180
#define SELECTOR_POSITIONS  3
#define SELECTOR_SPACING    (SELECTOR_LIMIT / (SELECTOR_POSITIONS - 1))
#define SELECTOR_POS_HOPPER (SELECTOR_SPACING*0)
#define SELECTOR_POS_SENSOR (SELECTOR_SPACING*1)
#define SELECTOR_POS_SORTER (SELECTOR_SPACING*2)

#define SELECTOR_JOSTLE_DEGREES      10
#define SELECTOR_JOSTLE_COUNT        10
#define SELECTOR_JOSTLE_DELAY        20

#define SORTER_LIMIT        180
#define SORTER_POSITIONS    6
#define SORTER_SPACING      (SORTER_LIMIT / (SORTER_POSITIONS - 1))
#define SORTER_POS_YELLOW   (SORTER_SPACING*0)
#define SORTER_POS_ORANGE   (SORTER_SPACING*1)
#define SORTER_POS_RED      (SORTER_SPACING*2)
#define SORTER_POS_GREEN    (SORTER_SPACING*3)
#define SORTER_POS_PURPLE   (SORTER_SPACING*4)
#define SORTER_POS_BLUE     (SORTER_SPACING*5)

#define SORTER_JOSTLE_DEGREES        4
#define SORTER_JOSTLE_COUNT          10
#define SORTER_JOSTLE_DELAY          20

typedef struct {
  float red;
  float green;
  float blue;
  int sorterPos;
  char *name;
} COLOR_PROFILE;

COLOR_PROFILE colors[] = {
  { 89.2, 102.3,  47.6, SORTER_POS_YELLOW, "Yellow M&M"},
  { 88.1, 106.7,  40.7, SORTER_POS_YELLOW, "Yellow Skittle"},
  {116.6,  69.8,  56.0, SORTER_POS_ORANGE, "Orange M&M"},
  {125.0,  69.4,  45.9, SORTER_POS_ORANGE, "Orange Skittle"},
  { 89.4,  83.2,  72.6, SORTER_POS_RED,    "Red M&M"},
  { 92.8,  83.7,  68.3, SORTER_POS_RED,    "Red Skittle"},
  { 57.2, 118.0,  64.9, SORTER_POS_GREEN,  "Green M&M"},
  { 57.3, 125.9,  54.2, SORTER_POS_GREEN,  "Green Skittle"},
  { 68.5,  92.2,  78.9, SORTER_POS_PURPLE, "Brown M&M"},
  { 67.0,  92.6,  80.4, SORTER_POS_PURPLE, "Purple Skittle"},
  { 39.7,  90.1, 112.5, SORTER_POS_BLUE,   "Blue M&M"}
};

#define COLORS            (sizeof(colors)/sizeof(COLOR_PROFILE))

#define PIN_LED               4
#define PIN_SERVO_SELECTOR    5
#define PIN_SERVO_SORTER      9

Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_4X);
Servo selector;
Servo sorter;

void enableSensorLed(bool on, int delayMs = 50)
{
  digitalWrite(PIN_LED, on ? HIGH : LOW);
  delay(delayMs);
}

void readColor(float *red, float *green, float *blue)
{
  enableSensorLed(true);
  tcs.setInterrupt(false);          //Start measurement
  delay(60);                        //Takes 50ms to read
  tcs.getRGB(red, green, blue);
  tcs.setInterrupt(true);
  enableSensorLed(false, 100);
}

float colorProfileDistance(COLOR_PROFILE *profile, float red, float green, float blue)
{
  return sqrt(sq(red - profile->red) + sq(green - profile->green) + sq(blue - profile->blue));
}

int bestColorProfile(float red, float green, float blue)
{
  float distance;
  float bestDistance = NULL;
  COLOR_PROFILE *bestProfile;
  int i;
  
  for (i = 0; i < COLORS; i++) {
    COLOR_PROFILE *profile = &colors[i];
    distance = colorProfileDistance(profile, red, green, blue);
    if (bestDistance == NULL || distance < bestDistance) {
      bestDistance = distance;
      bestProfile = profile;
    }
  }

  return bestProfile;
}

void logColor(float red, float green, float blue, COLOR_PROFILE *colorProfile)
{
  float distance = colorProfileDistance(colorProfile, red, green, blue);

  Serial.print("red = ");
  Serial.print(red, 2);
  Serial.print(", green = ");
  Serial.print(green, 2);
  Serial.print(", blue = ");
  Serial.print(blue, 2);
 
  Serial.print(": matched color = ");
  Serial.print(colorProfile->name);

  Serial.print(", color distance = ");
  Serial.print(distance, 2);
  Serial.println();
}

void moveSelector (int pos, int delayMs = 500)
{
  selector.write(pos);
  delay(delayMs);
}

void moveSorter (int pos, int delayMs = 500)
{
  sorter.write(pos);
  delay(delayMs);
}

void jostleSorter(int pos, int jostle = SORTER_JOSTLE_DEGREES, int count = SORTER_JOSTLE_COUNT, int delayMs = SORTER_JOSTLE_DELAY)
{
  int centerPos = (pos < SORTER_LIMIT) ? pos : pos - jostle;
  for (int i = 0; i < count; i++) {
    moveSorter(centerPos + jostle, delayMs);
    moveSorter(centerPos - jostle, delayMs);
  } 
  moveSorter(pos, delayMs);
}

void homeSelector()
{
  moveSelector(SELECTOR_POS_SORTER);
}

void jostleSelector(int pos, int jostle = SELECTOR_JOSTLE_DEGREES, int count = SELECTOR_JOSTLE_COUNT, int delayMs = SELECTOR_JOSTLE_DELAY)
{
  int centerPos = (pos < SELECTOR_LIMIT) ? pos : pos - jostle;
  for (int i = 0; i < count; i++) {
    moveSelector(pos + jostle, delayMs);
    moveSelector(pos, delayMs);
  } 
}

void randomSorter()
{
  int sorterIndex, sorterPos;
  
  sorterIndex = random(0, 6);
  sorterPos = colors[sorterIndex].sorterPos;
  Serial.print("Selector to ");
  Serial.print(sorterIndex);
  Serial.print(", ");
  Serial.print(sorterPos);
  Serial.println();
  moveSorter(sorterPos, 1000);
}

void setup()
{
  Serial.begin(115200);
  Serial.println("\nSkittles Colour Sorter");
  
  pinMode (PIN_LED, OUTPUT); 
  selector.attach(PIN_SERVO_SELECTOR);
  sorter.attach(PIN_SERVO_SORTER);
  tcs.begin();
  enableSensorLed(false);
}

void loop()
{
  // Test/calibration code...
  // randomSorter();
  // homeSelector();
  // return;

  // Regular code follows...

  // Load next candy
  moveSelector(SELECTOR_POS_HOPPER);
  jostleSelector(SELECTOR_POS_HOPPER);

  // Measure color
  moveSelector(SELECTOR_POS_SENSOR);
  
  float red, green, blue;
  readColor(&red, &green, &blue);

  COLOR_PROFILE *colorProfile;
  colorProfile = bestColorProfile(red, green, blue);
  logColor(red, green, blue, colorProfile);

  // Deliver candy to sorter
  moveSorter(colorProfile->sorterPos);
  moveSelector(SELECTOR_POS_SORTER);
  jostleSorter(colorProfile->sorterPos);
}

// end of file
