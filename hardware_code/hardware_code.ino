#include <DHT.h>

#define DHT_PIN     4
#define DHT_TYPE    DHT11
#define BUTTON_PIN  16
#define RELAY_PIN   27

#define BUTTON_PRESSED_LEVEL LOW

// Keep these values according to your relay test
#define RELAY_ON    HIGH
#define RELAY_OFF   LOW

// Hysteresis thresholds
const float TEMP_ON     = 30.0;
const float HUMIDITY_ON = 70.0;

const float TEMP_OFF     = 29.0;
const float HUMIDITY_OFF = 65.0;

// How long the fan stays off after the button is pressed
// while the sensor condition is still calling for the fan
const unsigned long FAN_RESUME_DELAY_MS = 30000;

DHT dht(DHT_PIN, DHT_TYPE);

bool fanOn = false;

// Button variables
int lastButtonReading = HIGH;
int stableButtonState = HIGH;

unsigned long lastDebounceTime = 0;
const unsigned long DEBOUNCE_MS = 50;

// Sensor variables
unsigned long lastSensorRead = 0;
const unsigned long SENSOR_INTERVAL_MS = 2000;

// Manual control variables
bool manualOverride = false;

// Automatic (hysteresis) state - what the sensors say the fan should be
bool autoFanState = false;
bool autoStateInitialized = false;

// 30-second "paused" timer
bool resumeTimerActive = false;
unsigned long resumeTimerStart = 0;


// =================================
// FAN CONTROL
// =================================

void setFan(bool on) {

  if (fanOn == on) {
    return;
  }

  fanOn = on;

  digitalWrite(RELAY_PIN, on ? RELAY_ON : RELAY_OFF);

  Serial.print("Fan: ");
  Serial.println(on ? "ON" : "OFF");
}


// =================================
// BUTTON STATUS
// =================================

const char* buttonText() {

  return (stableButtonState == BUTTON_PRESSED_LEVEL)
         ? "PRESSED" : "NOT PRESSED";
}


// =================================
// SETUP
// =================================

void setup() {

  Serial.begin(115200);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  digitalWrite(RELAY_PIN, RELAY_OFF);
  pinMode(RELAY_PIN, OUTPUT);

  dht.begin();

  lastButtonReading = digitalRead(BUTTON_PIN);
  stableButtonState = lastButtonReading;

  Serial.println("Smart Ventilation Fan Started");
  Serial.println("Initial Fan Status: OFF");

  Serial.print("Button Status: ");
  Serial.println(buttonText());
}


// =================================
// MAIN LOOP
// =================================

void loop() {

  // ---------------------------------
  // 1. PUSH BUTTON CONTROL
  // ---------------------------------

  int reading = digitalRead(BUTTON_PIN);

  if (reading != lastButtonReading) {
    lastDebounceTime = millis();
  }

  if (millis() - lastDebounceTime > DEBOUNCE_MS) {

    if (reading != stableButtonState) {

      stableButtonState = reading;

      Serial.print("BUTTON: ");
      Serial.println(buttonText());

      // Detect button press
      if (stableButtonState == BUTTON_PRESSED_LEVEL) {

        manualOverride = true;

        if (fanOn && autoFanState) {

          // Fan is ON because of temperature/humidity:
          // turn it off now, and bring it back after 30 seconds
          setFan(false);

          resumeTimerActive = true;
          resumeTimerStart = millis();

          Serial.println("Manual OFF: fan will resume in 30 seconds");
        }
        else {

          // Normal manual toggle (cancels any pending resume)
          resumeTimerActive = false;
          setFan(!fanOn);

          Serial.println("Manual Control Activated");
        }
      }
    }
  }

  lastButtonReading = reading;


  // ---------------------------------
  // 1b. 30-SECOND RESUME TIMER
  // ---------------------------------

  if (resumeTimerActive &&
      (millis() - resumeTimerStart >= FAN_RESUME_DELAY_MS)) {

    resumeTimerActive = false;
    manualOverride = false;   // back to automatic mode

    if (autoFanState) {
      Serial.println("30 seconds elapsed, condition still active");
      Serial.println("Automatic Control: Fan ON");
      setFan(true);
    }
    else {
      Serial.println("30 seconds elapsed, condition cleared: fan stays OFF");
    }
  }


  // ---------------------------------
  // 2. DHT11 SENSOR READINGS
  // ---------------------------------

  if (millis() - lastSensorRead >= SENSOR_INTERVAL_MS) {

    lastSensorRead = millis();

    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();

    Serial.println("-------------------------");

    Serial.print("Button Status: ");
    Serial.println(buttonText());

    Serial.print("Fan Status: ");
    Serial.println(fanOn ? "ON" : "OFF");

    if (isnan(humidity) || isnan(temperature)) {

      Serial.println("DHT11 sensor reading failed!");

    }
    else {

      Serial.print("Temperature: ");
      Serial.print(temperature);
      Serial.println(" C");

      Serial.print("Humidity: ");
      Serial.print(humidity);
      Serial.println(" %");


      // ---------------------------------
      // 3. AUTOMATIC CONTROL (HYSTERESIS)
      // ---------------------------------

      bool previousAutoState = autoFanState;

      if (temperature > TEMP_ON || humidity > HUMIDITY_ON) {
        autoFanState = true;
      }
      else if (temperature <= TEMP_OFF && humidity <= HUMIDITY_OFF) {
        autoFanState = false;
      }
      // Otherwise: inside the hysteresis band -> keep previous state


      // First valid sensor reading
      if (!autoStateInitialized) {

        autoStateInitialized = true;

        if (!manualOverride) {
          Serial.println(autoFanState ? "Automatic Control: Fan ON"
                                      : "Automatic Control: Fan OFF");
          setFan(autoFanState);
        }

      }

      // Automatic state changed -> return control to automatic mode
      else if (autoFanState != previousAutoState) {

        Serial.println("Hysteresis threshold crossed!");

        manualOverride = false;
        resumeTimerActive = false;   // cancel pending resume

        if (autoFanState) {
          Serial.println("Temp > 30 C or Humidity > 70 %");
          Serial.println("Automatic Control: Fan ON");
        }
        else {
          Serial.println("Temp <= 29 C and Humidity <= 65 %");
          Serial.println("Automatic Control: Fan OFF");
        }

        setFan(autoFanState);

      }

      // Normal automatic operation
      else if (!manualOverride) {

        if (fanOn != autoFanState) {
          Serial.println(autoFanState ? "Automatic Control: Fan ON"
                                      : "Automatic Control: Fan OFF");
          setFan(autoFanState);
        }

      }

      if (resumeTimerActive) {
        unsigned long elapsed = millis() - resumeTimerStart;
        unsigned long remaining = (elapsed < FAN_RESUME_DELAY_MS)
                                  ? (FAN_RESUME_DELAY_MS - elapsed) / 1000
                                  : 0;
        Serial.print("Fan paused, resumes in ~");
        Serial.print(remaining);
        Serial.println(" s");
      }
      else if (manualOverride) {
        Serial.println("Manual Override: ACTIVE");
      }
      else {
        Serial.println("Control Mode: AUTOMATIC");
      }
    }
  }
}
