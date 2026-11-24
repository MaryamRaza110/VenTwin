
#include <DHT.h>

#define DHT_PIN     4
#define DHT_TYPE    DHT11
#define BUTTON_PIN  16
#define RELAY_PIN   27

#define BUTTON_PRESSED_LEVEL LOW

// Keep these values according to your relay test
#define RELAY_ON    HIGH
#define RELAY_OFF   LOW

#define TEMP_THRESHOLD 30.0

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

// Temperature tracking
bool tempStateInitialized = false;
bool wasAboveThreshold = false;


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

        // Enable manual priority
        manualOverride = true;

        // Toggle fan
        setFan(!fanOn);

        Serial.println("Manual Control Activated");
      }
    }
  }

  lastButtonReading = reading;


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
      // 3. AUTOMATIC TEMPERATURE CONTROL
      // ---------------------------------

      bool aboveThreshold =
          (temperature > TEMP_THRESHOLD);


      // First valid sensor reading
      if (!tempStateInitialized) {

        wasAboveThreshold = aboveThreshold;
        tempStateInitialized = true;

        if (!manualOverride) {

          if (aboveThreshold) {
            Serial.println("Temperature above 30 C!");
            setFan(true);
          }
          else {
            setFan(false);
          }
        }

      }

      // Temperature has crossed 30 C
      else if (aboveThreshold != wasAboveThreshold) {

        wasAboveThreshold = aboveThreshold;

        Serial.println("Temperature threshold changed!");

        // Return control to automatic mode
        manualOverride = false;

        if (aboveThreshold) {

          Serial.println("Temperature above 30 C!");
          Serial.println("Automatic Control: Fan ON");

          setFan(true);

        }

        else {

          Serial.println("Temperature at or below 30 C!");
          Serial.println("Automatic Control: Fan OFF");

          setFan(false);

        }

      }

      // Normal automatic operation
      else if (!manualOverride) {

        if (aboveThreshold && !fanOn) {

          Serial.println("Automatic Control: Fan ON");
          setFan(true);

        }

        else if (!aboveThreshold && fanOn) {

          Serial.println("Automatic Control: Fan OFF");
          setFan(false);

        }

      }

      if (manualOverride) {
        Serial.println("Manual Override: ACTIVE");
      }
      else {
        Serial.println("Control Mode: AUTOMATIC");
      }

    }
  }
}
