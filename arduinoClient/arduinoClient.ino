#include "WiFi.h"
#include "HTTPClient.h"
#include "secrets.h"

const int testing = false;
const int noisy = false;

/*

Pin definitions

*/

const int LED_PIN = 2;
const int MOTION_PIN = 26;
const int MOTION_PIN2 = 22;
const int BUTTON_PIN = 17;
const int ALARM_PIN = 16;
const int BUTTON_LED_PIN = 21;

/*

Global constants related to alarm trip and reset.
Idea: keep N recent motion events; if the collected difference in time is less than X, then trip.
Idea: have a second sensor (one from front, one from side). When both sense movement, then trip.
Idea: only trip after two valid trips occur within given time.

*/
// Number of seconds after button press before reset system.
const int SEC_TO_RESET = ((testing) ? 1 : 10);

/*
Experimental results:
LOOP_DELAY(ms)  MOTION_DELAY(ms)    NUM_MOTIONS   TRIP_DIFF(sec)      RESULT
100             1000                7             7                   slightly difficult to trip, no false positives (few hours, daytime)
100             1000                7             5                   Too difficult to trip. Had to get super close and wait.
100             1000                7             8                   Difficult to trip, but had false positives...
50              1000                7             5                   Trips quite well, but has false positives.
50              500                 7             5                   


*/
// Maximum allowed milliseconds between valid motion events.
const int MOTION_DELAY = 500;
// Number of valid consecutive motion events required to trip alarm.
const int NUM_MOTIONS_TO_TRIP = 2;
// Number of milliseconds between valid "trips" in order to actually cause the alarm to go off.
const int TRIP_DIFF_TO_ALARM = 5 * 1000;
const int LOOP_DELAY = 50;

/*

Variables related to alarm tripping.

*/
// Holds recent valid motion events.
int motionTimes[NUM_MOTIONS_TO_TRIP];
int motionIndex = 0;
int tripTimes[2];
int tripIndex = 0;
bool alarmIsSounding = false;

/* Button LED state */
int blState = 0;
/* Number of loops to keep button LED state */
int blRatio = 10;
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

void buzzerOn() {
  if (noisy) {
    digitalWrite(ALARM_PIN, 1);
  }
}
void buzzerOff() {
  digitalWrite(ALARM_PIN, 0);
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
  WiFi.begin(ssid_NQ, password_NQ);
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
}

void setup() {
  Serial.begin(115200);
  pinMode(MOTION_PIN, INPUT_PULLUP);
  pinMode(MOTION_PIN2, INPUT_PULLUP);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_LED_PIN, OUTPUT);
  pinMode(ALARM_PIN, OUTPUT);
  if (connectToNetwork()) {
    Serial.println("Connection successful!");
    blinkLight(testing ? 3 : 20);
  } else {
    Serial.println("Connection failed!");
    writeLED(1);
  }
}

// Trip the alarm
bool soundAlarm(bool isTripped) {
  if (!isTripped) {
    return isTripped;
  }
  tripTimes[tripIndex] = millis();
  if (tripIndex == 0) {
    tripIndex = 1;
    return isTripped;
  }
  // If it's been too long since the previous trip, then replace the first one with this one (trip index set to 0).
  if (tripTimes[1] - tripTimes[0] > TRIP_DIFF_TO_ALARM) {
    tripIndex = 1;
    tripTimes[0] = tripTimes[1];
    return isTripped;
  }
  Serial.println("Alarm has been tripped.");
  alarmIsSounding = true;
  if (!testing) {
    getSite("https://us-central1-discolemur-info.cloudfunctions.net/tripAlarm");
  }
  buzzerOn();
  writeLED(1);
  return isTripped;
}

// Reset the trip
void resetAlarm() {
  buzzerOff();
  blinkLight(2);
  motionIndex = 0;
  writeLED(1);
  loopCounter = 0;
  tripIndex = 0;
  blState = 1;
  alarmIsSounding = false;
  Serial.print("Alarm has been reset: ");
  Serial.print(SEC_TO_RESET);
  Serial.println(" seconds until armed.");
  delay(1000 * SEC_TO_RESET);
}

// Handle a new motion event
// By the end of this function, motionIndex will be one number higher.
bool handleMotion() {
  int motionTime = millis();
  // If this motion is associated with the previous one, progress the index.
  if (motionTime - motionTimes[motionIndex - 1] < MOTION_DELAY || motionIndex == 0) {
    motionTimes[motionIndex] = motionTime;
    motionIndex++;
  } else {
    // Since this motion was not associated with the previous one, reset the index with this motion time.
    motionTimes[0] = motionTime;
    motionIndex = 1;
  }
  bool isTripped = (motionIndex == NUM_MOTIONS_TO_TRIP);
  if (isTripped) {
    motionIndex = 0;
  }
  // When the index has progressed to the number of required motions, then consider the alarm tripped.
  return isTripped;
}

void runMotionAlarm() {
  // After one minute, reset the alarm.
  if (millis() - tripTimes[tripIndex] > 60000 && alarmIsSounding) {
    Serial.println("It's been a minute, so reset.");
    resetAlarm();
  }
  // Change LED state when alarm isn't sounding and blRatio loops have passed.
  if (loopCounter % blRatio == 0 && !alarmIsSounding) {
    blState = (blState + 1) % 2;
    writeLED(blState);
  }
  int resetPressed = !digitalRead(BUTTON_PIN);
  int motionDetected = !digitalRead(MOTION_PIN) && !digitalRead(MOTION_PIN2);
  // Handle motion only when it hasn't been tripped.
  if (!alarmIsSounding) {
    Serial.println(motionDetected);
  }
  if (motionDetected && !alarmIsSounding) {
    soundAlarm(handleMotion());
    Serial.print("Motion index ");
    Serial.print(motionIndex);
    Serial.print(", Trip index ");
    Serial.println(tripIndex);
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
  //writeLED(motionDetected);
  delay(LOOP_DELAY);
}
