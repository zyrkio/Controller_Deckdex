#include <TMCStepper.h>
#include <SoftwareSerial.h>

// === Pinbelegung ===
#define UART_TX     10      // Arduino TX → TMC2209 PDN_UART
#define UART_RX     11      // Dummy (nicht verbunden)
#define DIAG_PIN    5       // DIAG1 → Arduino DigitalPin
#define R_SENSE     0.11f
#define DRIVER_ADDR 0b00    // MS1 + MS2 = GND

// === TMC2209 Objekt ===
SoftwareSerial SoftSerial(UART_RX, UART_TX);
TMC2209Stepper driver(&SoftSerial, R_SENSE, DRIVER_ADDR);

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("🔍 Starte StallGuard + DIAG1 Debug...");

  SoftSerial.begin(19200);                 // stabile Baudrate für SoftwareSerial
  driver.beginSerial(19200);

  pinMode(DIAG_PIN, INPUT_PULLUP);         // DIAG1 ist ein digitaler Ausgang → Input für Arduino

  // === Treiber initialisieren ===
  driver.begin();
  driver.toff(5);                          // Einschaltzeit für Treiber
  driver.rms_current(900);                // Setze Strom (an Motor anpassen!)
  driver.microsteps(16);
  driver.intpol(false);

  // === StallGuard Setup ===
  driver.en_spreadCycle(true);            // SpreadCycle aktivieren → notwendig für StallGuard
  driver.pwm_autoscale(false);            // StealthChop deaktivieren!
  driver.SGTHRS(10);                      // StallGuard-Empfindlichkeit (0 = sehr empfindlich)
  driver.TCOOLTHRS(0xFFFFF);              // StallGuard immer aktiv

  // === DIAG1 = Stall konfigurieren (LOW = Stall erkannt)
  uint32_t gconf = driver.GCONF();
  gconf |= (1 << 11);  // diag1_stall aktiv
  gconf &= ~(1 << 4);  // diag1_active_high = 0 → DIAG1 LOW bei Stall
  driver.GCONF(gconf);
  driver.push();       // Konfiguration senden

  Serial.println("✅ Konfiguration abgeschlossen");
  Serial.println("🔧 Überprüfe DIAG1-Pegel und Registerwerte...");
  Serial.println();
}

void loop() {
  static unsigned long last = 0;
  if (millis() - last >= 1000) {
    // === DIAG1 auslesen ===
    bool diag = digitalRead(DIAG_PIN);

    Serial.print("DIAG1 Zustand: ");
    Serial.println(diag ? "HIGH (kein Stall)" : "LOW (STALL erkannt)");

    // === Registerwerte prüfen ===
    Serial.print("SGTHRS: ");
    Serial.println(driver.SGTHRS());

    Serial.print("TCOOLTHRS: ");
    Serial.println(driver.TCOOLTHRS());

    Serial.print("SpreadCycle aktiv?: ");
    Serial.println(driver.en_spreadCycle() ? "JA" : "NEIN");

    Serial.print("PWM Autoscale (StealthChop): ");
    Serial.println(driver.pwm_autoscale() ? "AKTIV" : "DEAKTIVIERT");

    Serial.print("GCONF: 0b");
    Serial.println(driver.GCONF(), BIN);

    Serial.println("--------------------------");
    last = millis();
  }
}
