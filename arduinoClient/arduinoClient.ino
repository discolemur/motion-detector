#include "WiFi.h"
#include "HTTPClient.h"
#include "secrets.h"

/*

Pin definitions

*/

const int LED_PIN = 2;
const int MOTION_PIN = 26;
const int BUTTON_PIN = 17;
const int ALARM_PIN = 16;
const int BUTTON_LED_PIN = 21;

/*

Global constants related to alarm trip and reset.

*/
// Number of seconds after button press before reset system.
const int SEC_TO_RESET = 10;
// Maximum allowed milliseconds between valid motion events.
const int MOTION_DELAY = 5000;
// Number of valid consecutive motion events required to trip alarm.
const int NUM_MOTIONS_TO_TRIP = 5;

/*

Variables related to alarm tripping.

*/
// Holds recent valid motion events.
int motionTimes[NUM_MOTIONS_TO_TRIP];
int motionIndex = 0;

/* Button LED state */
int blState = 0;
/* Number of loops to keep button LED state */
int blRatio = 5;
/* Loop counter */
int loopCounter = 0;

/*

Functions for main program!

*/

// Writes value to both LEDs: LED_PIN and BUTTON_LED_PIN.
void writeLED(int val) {
  digitalWrite(LED_PIN, val);
  digitalWrite(BUTTON_LED_PIN, val);
}

void blinkLight(int times) {
  for (int i = 0; i < times; ++i) {
    writeLED(0);
    delay(250);
    writeLED(1);
    delay(250);
  }
  writeLED(0);
}

bool connectToNetwork() {
  Serial.println("Connecting to network...");
  // NOTE: ssid and password must be set properly when moved location.
  WiFi.begin(ssid_house_home, password_house_home);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Still trying to connect to wifi...");
  }
  return WiFi.status() == WL_CONNECTED;
}

void getSite(const char* site) { 
  if ((WiFi.status() != WL_CONNECTED)) {
    return;
  }
  Serial.println(site);
  HTTPClient http;
  if (root_ca != "") {
    http.begin(site, root_ca);
  } else {
    http.begin(site);
  }
  int httpCode = http.GET();
  if (httpCode > 0) { //Check for the returning code
    String payload = http.getString();
    Serial.println(httpCode);
    Serial.println(payload);
  }else {
    Serial.println("Error on HTTP request");
  }
  http.end();
  // delay(10000);
}

void setup() {
  Serial.begin(115200);
  pinMode(MOTION_PIN, INPUT_PULLUP);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_LED_PIN, OUTPUT);
  pinMode(ALARM_PIN, OUTPUT);
  if (connectToNetwork()) {
    Serial.println("Connection successful!");
    blinkLight(4);
  } else {
    Serial.println("Connection failed!");
    writeLED(1);
  }
}

// Trip the alarm
void soundAlarm(bool isTripped) {
  if (isTripped) {
    getSite("https://us-central1-discolemur-info.cloudfunctions.net/tripAlarm");
    writeLED(1);
    digitalWrite(ALARM_PIN, 1);
  }
}

// Reset the trip
void resetAlarm() {
  digitalWrite(ALARM_PIN, 0);
  blinkLight(2);
  motionIndex = 0;
  writeLED(1);
  loopCounter = 0;
  blState = 1;
  delay(1000 * SEC_TO_RESET);
}

// Handle a new motion event
// By the end of this function, motionIndex will be one number higher.
bool handleMotion() {
  int motionTime = millis();
  if (motionIndex == 0) {
    // First motion since reset should be saved.
    motionTimes[0] = motionTime;
    motionIndex++;
  } else if (motionTime - motionTimes[motionIndex - 1] < MOTION_DELAY) {
    // If this motion is associated with the previous one, progress the index.
    motionTimes[motionIndex] = motionTime;
    motionIndex++;
  } else {
    // Since this motion was not associated with the previous one, reset the index with this motion time.
    motionTimes[0] = motionTime;
    motionIndex = 1;
  }
  // When the index has progressed to the number of required motions, then consider the alarm tripped.
  return motionIndex >= NUM_MOTIONS_TO_TRIP;
}

void runMotionAlarm() {
  // Change LED state when hasn't tripped and blRatio loops have passed.
  if (loopCounter % blRatio == 0 && motionIndex < NUM_MOTIONS_TO_TRIP) {
    blState = (blState + 1) % 2;
    writeLED(blState);
  }
  int resetPressed = !digitalRead(BUTTON_PIN);
  int motionDetected = !digitalRead(MOTION_PIN);
  // Handle motion only when it hasn't been tripped.
  if (motionDetected && motionIndex < NUM_MOTIONS_TO_TRIP) {
    soundAlarm(handleMotion());
  }
  // Reset the trip state when the button is pressed.
  if (resetPressed) {
    resetAlarm();
  }
  loopCounter++;
}

// Main loop
void loop() {
  runMotionAlarm();
  delay(50);
}
