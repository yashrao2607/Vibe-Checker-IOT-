/*
 ============================================================
   DIGITAL CROWD MOOD ESTIMATOR — PRESENTATION MODE
   Automated Scenario Logic for End-Sem Demo
 ============================================================
*/

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

const int SOUND_PIN = A0;
const int LED_CALM = 8, LED_ACTIVE = 9, LED_EXCITED = 10, LED_CHAOTIC = 11;
const int BUZZER_PIN = 12;

LiquidCrystal_I2C lcd(0x27, 16, 2); 

void setup() {
  Serial.begin(115200);
  
  // Auto-Detect I2C Address (0x27 or 0x3F)
  Wire.begin();
  Wire.beginTransmission(0x27);
  if (Wire.endTransmission() != 0) {
    lcd = LiquidCrystal_I2C(0x3F, 16, 2);
  }

  lcd.init();
  lcd.backlight();
  
  pinMode(LED_CALM, OUTPUT); 
  pinMode(LED_ACTIVE, OUTPUT);
  pinMode(LED_EXCITED, OUTPUT); 
  pinMode(LED_CHAOTIC, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  // Initial Splash
  lcd.setCursor(1, 0); lcd.print("CROWD ANALYZER");
  lcd.setCursor(2, 1); lcd.print("v2.0 ONLINE");
  delay(2000);
  lcd.clear();
}

void loop() {
  unsigned long runTime = millis();
  int simulatedVal = 0;
  String currentMood = "";
  String desc = "";

  // 🎭 THE AUTOMATED DEMO SCRIPT
  // 0s - 4s: Starting state
  if (runTime < 4000) {
    simulatedVal = 150; 
    currentMood = "CALM   ";
    desc = "Safe & Quiet   ";
  } 
  // 4s - 8s: Crowd gathering
  else if (runTime < 8000) {
    simulatedVal = 450; 
    currentMood = "ACTIVE ";
    desc = "Lively Crowd   ";
  }
  // 8s - 12s: High excitement
  else if (runTime < 12000) {
    simulatedVal = 750; 
    currentMood = "EXCITED";
    desc = "Loud Energy!   ";
  }
  // 12s - 20s: Riot / Danger state
  else if (runTime < 20000) {
    simulatedVal = 980; 
    currentMood = "CHAOTIC";
    desc = "DANGER: ALERT  ";
  }
  // 20s+: Reset to normal
  else {
    simulatedVal = 120; 
    currentMood = "CALM   ";
    desc = "Issue Resolved ";
  }

  // Update Display
  lcd.setCursor(0, 0);
  lcd.print("MOOD: ");
  lcd.print(currentMood);
  
  lcd.setCursor(0, 1);
  lcd.print(desc);

  // Update LEDs & Buzzer based on the Simulation
  digitalWrite(LED_CALM,    (simulatedVal < 300) ? HIGH : LOW);
  digitalWrite(LED_ACTIVE,  (simulatedVal >= 300 && simulatedVal < 600) ? HIGH : LOW);
  digitalWrite(LED_EXCITED, (simulatedVal >= 600 && simulatedVal < 900) ? HIGH : LOW);
  digitalWrite(LED_CHAOTIC, (simulatedVal >= 900) ? HIGH : LOW);

  // Siren alert for Chaotic
  if (simulatedVal >= 900) {
    // Pulsing tone to get attention
    tone(BUZZER_PIN, (runTime % 1000 < 500) ? 1000 : 800); 
  } else {
    noTone(BUZZER_PIN);
  }

  // Data Log for Serial Monitor
  Serial.print("[SIMULATION] Energy Level: "); 
  Serial.print(map(simulatedVal, 0, 1023, 0, 100));
  Serial.print("% | Current State: ");
  Serial.println(currentMood);
  
  delay(100);
}
