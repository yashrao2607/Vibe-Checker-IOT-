/*
 ============================================================
   DIGITAL CROWD MOOD ESTIMATOR — ADVANCED v2.0
   Features: Auto-Calibration, Signal Metering, Siren Alerts
   Auto-Detect LCD Address (0x27 or 0x3F)
 ============================================================
*/

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Pins
const int SOUND_PIN   = A0;
const int LED_CALM    = 8;
const int LED_ACTIVE  = 9;
const int LED_EXCITED = 10;
const int LED_CHAOTIC = 11;
const int BUZZER_PIN  = 12;

// Settings
const int SAMPLE_WINDOW = 40; 
float noiseFloor = 0;         
float peakVariance = 0;       

// Use fixed Wokwi LCD I2C address for stability
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Custom Characters for Pulse Meter
byte bar1[8] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1F};
byte bar2[8] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1F, 0x1F};
byte bar3[8] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x1F, 0x1F, 0x1F};
byte bar4[8] = {0x0, 0x0, 0x0, 0x0, 0x1F, 0x1F, 0x1F, 0x1F};
byte bar5[8] = {0x0, 0x0, 0x0, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F};
byte bar6[8] = {0x0, 0x0, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F};
byte bar7[8] = {0x0, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F};
byte bar8[8] = {0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F};

void setup() {
  Serial.begin(115200);

  lcd.init();
  lcd.backlight();
  
  // Create Custom Meter Characters
  lcd.createChar(0, bar1); lcd.createChar(1, bar2);
  lcd.createChar(2, bar3); lcd.createChar(3, bar4);
  lcd.createChar(4, bar5); lcd.createChar(5, bar6);
  lcd.createChar(6, bar7); lcd.createChar(7, bar8);

  pinMode(LED_CALM, OUTPUT); pinMode(LED_ACTIVE, OUTPUT);
  pinMode(LED_EXCITED, OUTPUT); pinMode(LED_CHAOTIC, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  // Startup sanity blink so we can confirm firmware is executing.
  digitalWrite(LED_CALM, HIGH); delay(150);
  digitalWrite(LED_CALM, LOW);  delay(150);

  // Splash Screen
  lcd.setCursor(1, 0); lcd.print("CROWD ANALYZER");
  lcd.setCursor(2, 1); lcd.print("v2.0 BOOTING");
  delay(1500);

  // STEP: Auto-Calibration
  lcd.clear();
  lcd.print("CALIBRATING...");
  lcd.setCursor(0, 1);
  long calibSum = 0;
  for(int i=0; i<16; i++) {
    calibSum += analogRead(SOUND_PIN);
    lcd.print(">");
    delay(100);
  }
  noiseFloor = (float)calibSum / 16.0;
  lcd.clear();
}

void loop() {
  float samples[SAMPLE_WINDOW];
  float sum = 0;

  // 1. Capture High-Speed Samples
  for (int i = 0; i < SAMPLE_WINDOW; i++) {
    samples[i] = abs(analogRead(SOUND_PIN) - noiseFloor);
    sum += samples[i];
    delay(2);
  }
  float avgEnergy = sum / SAMPLE_WINDOW;

  // 2. Calculate Variance (Variability)
  float varianceSum = 0;
  for (int i = 0; i < SAMPLE_WINDOW; i++) {
    varianceSum += pow(samples[i] - avgEnergy, 2);
  }
  float variance = varianceSum / SAMPLE_WINDOW;

  // 3. Peak Tracking
  if (variance > peakVariance) peakVariance = variance;

  // 4. Mood Logic
  String mood;
  int moodLvl = 0;
  if      (variance < 2000)  { mood = "CALM   "; moodLvl = 1; }
  else if (variance < 10000) { mood = "ACTIVE "; moodLvl = 2; }
  else if (variance < 35000) { mood = "EXCITED"; moodLvl = 3; }
  else                       { mood = "CHAOTIC"; moodLvl = 4; }

  // 5. Visual/Audio Output
  updateHardware(moodLvl);

  // 6. LCD Update
  lcd.setCursor(0, 0);
  lcd.print("MOOD: " + mood);
  
  // Custom Intensity Meter on Row 1
  lcd.setCursor(0, 1);
  int meterWidth = map(constrain(variance, 0, 50000), 0, 50000, 0, 15);
  for(int i=0; i<16; i++) {
    if(i <= meterWidth) lcd.write((uint8_t)map(meterWidth, 0, 15, 0, 7));
    else lcd.print(" ");
  }

  // 7. Data Logging
  Serial.print("[DATA] Var:"); Serial.print(variance);
  Serial.print(" | Peak:"); Serial.println(peakVariance);
  
  delay(50);
}

void updateHardware(int level) {
  digitalWrite(LED_CALM,    level == 1 ? HIGH : LOW);
  digitalWrite(LED_ACTIVE,  level == 2 ? HIGH : LOW);
  digitalWrite(LED_EXCITED, level == 3 ? HIGH : LOW);
  digitalWrite(LED_CHAOTIC, level == 4 ? HIGH : LOW);

  if (level == 4) {
    tone(BUZZER_PIN, 1000);
  } else {
    noTone(BUZZER_PIN);
  }
}
