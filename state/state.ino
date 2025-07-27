//Smart State Detection System

//A system that intelligently monitors air quality, temperature, and humidity.

//@A.I Instaraj

#include <DHT.h>

#define MQ2_PIN A0
#define DHT_PIN 2
#define DHT_TYPE DHT11  

#define GREEN_LED 3
#define RED_LED 4
#define BUZZER 5

DHT dht(DHT_PIN, DHT_TYPE);

// === Different States

//✅ Safe

//⚠️ Warning

//🚨 Danger

//☠️ Critical

enum State { SAFE, WARNING, DANGER, CRITICAL };
State currentState = SAFE;
State previousState = SAFE;

unsigned long lastCheckTime = 0;
unsigned long lastWarningTime = 0;
unsigned long lastDangerTime = 0;
unsigned long safeStartTime = 0;

// === STATE DURATION SETTINGS (in ms) ===
const unsigned long CHECK_INTERVAL = 5000;
const unsigned long WARNING_DURATION = 30000;    
const unsigned long SAFE_COOLDOWN = 20000;       

// === THRESHOLDS ===
const int GAS_WARNING = 200;
const int GAS_DANGER = 400;
const int GAS_CRITICAL = 600;

const float TEMP_SAFE_MIN = 20.0;
const float TEMP_SAFE_MAX = 30.0;
const float TEMP_DANGER = 40.0;

const float HUMID_SAFE_MIN = 40.0;
const float HUMID_SAFE_MAX = 60.0;
const float HUMID_DANGER_LOW = 20.0;
const float HUMID_DANGER_HIGH = 80.0;

// === BUFFER FOR PREVIOUS READINGS ===
int prevGas = 0;
float prevTemp = 0;
float prevHumid = 0;

void setup() {
  Serial.begin(9600);
  dht.begin();
  
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, LOW);
  digitalWrite(BUZZER, LOW);

  lastCheckTime = millis();
  safeStartTime = millis();
}

void loop() {
  if (millis() - lastCheckTime >= CHECK_INTERVAL) {
    lastCheckTime = millis();
    evaluateEnvironment();
  }

  updateOutputs();
}

// === FUNCTION: Evaluate Environment and Decide State ===
void evaluateEnvironment() {
  int gas = analogRead(MQ2_PIN);
  float temp = dht.readTemperature();
  float humid = dht.readHumidity();

  Serial.print("Gas: "); 
  Serial.print(gas);
  Serial.print(" | Temp: "); 
  Serial.print(temp);
  Serial.print(" | Humidity: "); 
  Serial.println(humid);

  bool gasRapidRise = (gas - prevGas > 150);
  bool tempSpike = (temp - prevTemp > 5);
  bool humidJump = abs(humid - prevHumid) > 20;

  prevGas = gas;
  prevTemp = temp;
  prevHumid = humid;

  // === CRITICAL ===
  if (gas > GAS_CRITICAL || (gas > GAS_DANGER && (tempSpike || gasRapidRise || humidJump))) {
    currentState = CRITICAL;
    return;
  }

  // === DANGER ===
  if (gas > GAS_DANGER || temp > TEMP_DANGER || humid < HUMID_DANGER_LOW || humid > HUMID_DANGER_HIGH) {
    currentState = DANGER;
    lastDangerTime = millis();
    return;
  }

  // === WARNING ===
  if (gas > GAS_WARNING || temp < TEMP_SAFE_MIN || temp > TEMP_SAFE_MAX || humid < HUMID_SAFE_MIN || humid > HUMID_SAFE_MAX) {
    currentState = WARNING;
    if (previousState != WARNING)
      lastWarningTime = millis();
    else if (millis() - lastWarningTime >= WARNING_DURATION) {
      currentState = DANGER;
    }
    return;
  }

  // === SAFE ===
  if (previousState != SAFE)
    safeStartTime = millis();
  else if (millis() - safeStartTime >= SAFE_COOLDOWN) {
    currentState = SAFE;
  } else {
    currentState = previousState; 
  }

  previousState = currentState;
}

// === React to State with LEDs and Buzzer ===
void updateOutputs() {
  switch (currentState) {
    case SAFE:
      digitalWrite(GREEN_LED, HIGH);
      digitalWrite(RED_LED, LOW);
      digitalWrite(BUZZER, LOW);
      break;

    case WARNING:
      blink(GREEN_LED, 800);
      digitalWrite(RED_LED, LOW);
      digitalWrite(BUZZER, LOW);
      break;

    case DANGER:
      digitalWrite(GREEN_LED, LOW);
      blink(RED_LED, 500);
      tone(BUZZER, 1000, 300);
      delay(300);
      break;

    case CRITICAL:
      digitalWrite(GREEN_LED, LOW);
      blink(RED_LED, 200);
      tone(BUZZER, 1500, 100);
      delay(150);
      break;
  }
}

// === BLINK
void blink(int pin, int interval) {
  digitalWrite(pin, (millis() / interval) % 2);
}
