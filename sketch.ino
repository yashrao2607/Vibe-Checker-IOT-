/*
 ============================================================
   DIGITAL CROWD MOOD ESTIMATOR — v3.1
   Potentiometer simulates crowd energy level (A0)
   Mood: CALM / ACTIVE / EXCITED / CHAOTIC
 ============================================================
*/

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ── Pin Definitions ──────────────────────────────────────
#define SOUND_PIN    A0
#define LED_CALM      8
#define LED_ACTIVE    9
#define LED_EXCITED  10
#define LED_CHAOTIC  11
#define BUZZER_PIN   12

// ── Constants ────────────────────────────────────────────
#define SAMPLE_COUNT  20

// ── Globals ──────────────────────────────────────────────
int   noiseFloor   = 0;
float peakEnergy   = 0;
int   lastMoodLvl  = -1;
unsigned long moodStart = 0;
int   sirenFreq    = 500;
bool  sirenDir     = true;

LiquidCrystal_I2C lcd(0x27, 16, 2);

// ── Custom full-block character (slot 0) ─────────────────
byte BLOCK[8] = {0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F};

// ─────────────────────────────────────────────────────────
void setup() {
  Serial.begin(9600);

  // LED + Buzzer pins
  pinMode(LED_CALM,    OUTPUT);
  pinMode(LED_ACTIVE,  OUTPUT);
  pinMode(LED_EXCITED, OUTPUT);
  pinMode(LED_CHAOTIC, OUTPUT);
  pinMode(BUZZER_PIN,  OUTPUT);

  // Quick LED startup test — proves wiring works
  digitalWrite(LED_CALM,    HIGH); delay(200);
  digitalWrite(LED_ACTIVE,  HIGH); delay(200);
  digitalWrite(LED_EXCITED, HIGH); delay(200);
  digitalWrite(LED_CHAOTIC, HIGH); delay(200);
  digitalWrite(LED_CALM,    LOW);
  digitalWrite(LED_ACTIVE,  LOW);
  digitalWrite(LED_EXCITED, LOW);
  digitalWrite(LED_CHAOTIC, LOW);

  // LCD init — delay required for I2C to settle in Wokwi
  delay(100);
  lcd.init();
  lcd.backlight();

  // Register custom block char in slot 0
  // Must call lcd.clear() after createChar to reset cursor
  lcd.createChar(0, BLOCK);
  lcd.clear();

  // Splash
  lcd.setCursor(0, 0);
  lcd.print(" CROWD ANALYZER ");
  lcd.setCursor(0, 1);
  lcd.print("   STARTING...  ");
  delay(1200);

  // Calibrate: read baseline when pot is at rest
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("CALIBRATING...");
  lcd.setCursor(0, 1);
  long sum = 0;
  for (int i = 0; i < 16; i++) {
    sum += analogRead(SOUND_PIN);
    lcd.write((byte)0); // progress blocks
    delay(80);
  }
  noiseFloor = (int)(sum / 16);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("BASE:");
  lcd.print(noiseFloor);
  lcd.setCursor(0, 1);
  lcd.print("  SYSTEM READY! ");
  delay(800);
  lcd.clear();

  moodStart = millis();
}

// ─────────────────────────────────────────────────────────
void loop() {
  // ── 1. Sample pot energy ──────────────────────────────
  long rawSum = 0;
  for (int i = 0; i < SAMPLE_COUNT; i++) {
    rawSum += abs(analogRead(SOUND_PIN) - noiseFloor);
    delay(2);
  }
  float energy = (float)rawSum / SAMPLE_COUNT;

  // ── 2. Peak hold with gentle decay ───────────────────
  if (energy > peakEnergy) peakEnergy = energy;
  else                     peakEnergy *= 0.993f;

  // ── 3. Energy % (0–100) ──────────────────────────────
  int pct = (int)(energy * 100.0f / 1023.0f);
  if (pct > 100) pct = 100;

  // ── 4. Mood detection (energy-based) ─────────────────
  const char* moodLabel;
  int moodLvl;
  if      (energy < 150) { moodLabel = "CALM   "; moodLvl = 1; }
  else if (energy < 400) { moodLabel = "ACTIVE "; moodLvl = 2; }
  else if (energy < 700) { moodLabel = "EXCITED"; moodLvl = 3; }
  else                   { moodLabel = "CHAOTIC"; moodLvl = 4; }

  // Mood-change event
  if (moodLvl != lastMoodLvl) {
    lastMoodLvl = moodLvl;
    moodStart   = millis();
    sirenFreq   = 500;
    sirenDir    = true;
  }
  int durSec = (int)((millis() - moodStart) / 1000UL);
  if (durSec > 999) durSec = 999;

  // ── 5. Update LEDs ───────────────────────────────────
  digitalWrite(LED_CALM,    moodLvl == 1 ? HIGH : LOW);
  digitalWrite(LED_ACTIVE,  moodLvl == 2 ? HIGH : LOW);
  digitalWrite(LED_EXCITED, moodLvl == 3 ? HIGH : LOW);
  digitalWrite(LED_CHAOTIC, moodLvl == 4 ? HIGH : LOW);

  // ── 6. LCD Row 0: mood + % + duration ────────────────
  // Format: "EXCITED 72% 12s"  (16 chars)
  lcd.setCursor(0, 0);
  lcd.print(moodLabel);          // 7 chars: "CALM   " etc
  lcd.print(' ');
  if (pct < 10)  lcd.print(' ');
  if (pct < 100) lcd.print(' ');
  lcd.print(pct);
  lcd.print('%');
  lcd.print(' ');
  if (durSec < 10)  lcd.print(' ');
  if (durSec < 100) lcd.print(' ');
  lcd.print(durSec);
  lcd.print('s');

  // ── 7. LCD Row 1: level meter + peak marker ──────────
  lcd.setCursor(0, 1);
  int filled = (int)((energy / 1023.0f) * 15.0f);
  if (filled > 15) filled = 15;
  int peak16 = (int)((peakEnergy / 1023.0f) * 15.0f);
  if (peak16 > 15) peak16 = 15;

  for (int i = 0; i < 16; i++) {
    if      (i < filled)                      lcd.write((byte)0); // filled block
    else if (i == peak16 && peak16 >= filled) lcd.print('|');     // peak marker
    else                                       lcd.print(' ');
  }

  // ── 8. Buzzer logic ──────────────────────────────────
  if (moodLvl == 4) {
    // Sweeping siren for CHAOTIC
    tone(BUZZER_PIN, sirenFreq);
    sirenFreq += sirenDir ? 20 : -20;
    if (sirenFreq >= 1100) sirenDir = false;
    if (sirenFreq <= 450)  sirenDir = true;
  } else if (moodLvl == 3) {
    // Short beep every 500ms for EXCITED
    static unsigned long lastBeep = 0;
    if (millis() - lastBeep > 500) {
      lastBeep = millis();
      tone(BUZZER_PIN, 880, 80);
    }
  } else {
    noTone(BUZZER_PIN);
  }

  // ── 9. Serial log ────────────────────────────────────
  Serial.print("E:"); Serial.print((int)energy);
  Serial.print(" %:"); Serial.print(pct);
  Serial.print(" Mood:"); Serial.print(moodLabel);
  Serial.print(" Dur:"); Serial.print(durSec);
  Serial.println("s");

  delay(80);
}
