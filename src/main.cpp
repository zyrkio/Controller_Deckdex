#include <TMCStepper.h>
#include <SoftwareSerial.h>

// === Pinbelegung ===
#define DIAG_PIN     2
#define UART_TX      10
#define UART_RX      11
#define STEP_PIN     3
#define DIR_PIN      5
#define R_SENSE      0.11f
#define DRIVER_ADDR  0b00

SoftwareSerial SoftSerial(UART_RX, UART_TX);
TMC2209Stepper driver(&SoftSerial, R_SENSE, DRIVER_ADDR);

// === Variablen
bool diag_state = false;
uint16_t sg_result = 0;

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("🔍 TMC2209 StallGuard Diagnose mit Schrittbewegung");

  pinMode(DIAG_PIN, INPUT);
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);

  SoftSerial.begin(19200);
  driver.beginSerial(19200);
  driver.begin();

  // --- CHOPCONF vor Konfiguration (zur Sicherheit)
  Serial.print("CHOPCONF (vor Setup): 0b");
  Serial.println(driver.CHOPCONF(), BIN);

  // === Treiber konfigurieren
  driver.toff(5);
  driver.rms_current(1500);       // Motorstrom
  driver.microsteps(16);
  driver.intpol(false);
  driver.en_spreadCycle(true);
  driver.pwm_autoscale(false);
  driver.TCOOLTHRS(0xFFFFF);
  driver.TPWMTHRS(0);
  driver.SGTHRS(10);

  // === GCONF setzen
  uint32_t gconf = driver.GCONF();
  gconf |= (1 << 11); // diag1_stall
  gconf &= ~(1 << 4); // active LOW
  gconf |= (1 << 2);  // pushpull
  driver.GCONF(gconf);
  driver.push();


  delay(100); // sicherstellen, dass Register gesetzt wurden

  // === DEBUG: Register und Status anzeigen
  Serial.println("✅ Konfiguration abgeschlossen:");
  Serial.print("TOFF: "); Serial.println(driver.toff());
  Serial.print("RMS Current: "); Serial.print(driver.rms_current()); Serial.println(" mA");
  Serial.print("Microsteps: "); Serial.println(driver.microsteps());
  Serial.print("SGTHRS: "); Serial.println(driver.SGTHRS());
  Serial.print("TCOOLTHRS: "); Serial.println(driver.TCOOLTHRS());
  Serial.print("TPWMTHRS: "); Serial.println(driver.TPWMTHRS());
  Serial.print("SpreadCycle aktiv: "); Serial.println(driver.en_spreadCycle() ? "JA" : "NEIN");
  Serial.print("StealthChop aktiv (pwm_autoscale): "); Serial.println(driver.pwm_autoscale() ? "JA" : "NEIN");
  Serial.print("GCONF: 0b"); Serial.println(driver.GCONF(), BIN);
  Serial.print("CHOPCONF (nach Setup): 0b"); Serial.println(driver.CHOPCONF(), BIN);
  Serial.print("DIAG1 Zustand: "); Serial.println(digitalRead(DIAG_PIN) ? "HIGH (kein Stall)" : "LOW (STALL!)");

  Serial.println("🏁 Start Testbewegung...");
  digitalWrite(DIR_PIN, HIGH);
}

void loop() {
  // Schrittimpuls generieren (konstante Geschwindigkeit)
  digitalWrite(STEP_PIN, HIGH);
  delayMicroseconds(500); // = 2kHz Step-Frequenz
  digitalWrite(STEP_PIN, LOW);
  delayMicroseconds(500);

  // SG_RESULT und DIAG1 anzeigen (alle 500 ms)
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint >= 500) {
    uint16_t sg_result = driver.SG_RESULT();
    bool diag_state = digitalRead(DIAG_PIN);
    uint8_t sgthrs = driver.SGTHRS();

    Serial.print("📊 DIAG1: ");
    Serial.print(diag_state ? "HIGH (kein Stall)" : "LOW (STALL!)");
    Serial.print(" | SG_RESULT: ");
    Serial.print(sg_result);
    Serial.print(" | SGTHRS*2 = ");
    Serial.print(sgthrs * 2);

    // Bewertung
    if (sg_result < sgthrs * 2) {
      Serial.print(" ❗ → Stall erkannt");
    } else {
      Serial.print(" ✅ → kein Stall");
    }

    Serial.println();
    lastPrint = millis();
  }
}