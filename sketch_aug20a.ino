#include <Wire.h>
#include <LiquidCrystal.h>
#include <Adafruit_TCS34725.h>
#include "colori.h"   // deve contenere trovaColore(...) e la palette

// === LCD: scegli la variante che corrisponde ai tuoi collegamenti ===
// Variante A (RS=7, E=8, D4=9, D5=10, D6=11, D7=12)
LiquidCrystal lcd(7, 8, 9, 10, 11, 12);

// Variante B (RS=12, E=11, D4=10, D5=9, D6=8, D7=7)
// LiquidCrystal lcd(12, 11, 10, 9, 8, 7);

// Sensore colore
Adafruit_TCS34725 tcs(
  TCS34725_INTEGRATIONTIME_50MS,
  TCS34725_GAIN_4X
);

// Bottone
const int BUTTON_PIN = 2;

// Debounce
const unsigned long DEBOUNCE_MS = 35;
bool lastReading = HIGH;
bool stableState = HIGH;
unsigned long lastChangeMs = 0;

// --- PROTOTIPO della funzione usata nel loop (risolve l'errore) ---
void eseguiLetturaColore();

static inline uint16_t clamp255u(uint32_t v){ return (v>255u)?255u:(uint16_t)v; }

void setup() {
  Serial.begin(115200);

  lcd.begin(16, 2);
  lcd.clear();
  lcd.print("Huemate Ready!");
  lcd.setCursor(0, 1);
  lcd.print("Premi il bottone");

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);

  // inizializza stati per debounce
  lastReading = digitalRead(BUTTON_PIN);
  stableState = lastReading;

  if (!tcs.begin()) {
    Serial.println("Errore: TCS34725 non trovato");
    lcd.clear(); lcd.print("Errore Sensore!");
    while (1) {}
  } else {
    Serial.println("TCS34725 OK");
  }
}

void loop() {
  // Debounce + edge su LOW (premuto)
  bool reading = digitalRead(BUTTON_PIN);

  if (reading != lastReading) {
    lastChangeMs = millis();
    lastReading = reading;
  }

  if (millis() - lastChangeMs > DEBOUNCE_MS) {
    if (reading != stableState) {
      stableState = reading;
      if (stableState == LOW) { // fronte di discesa
        digitalWrite(LED_BUILTIN, HIGH);
        Serial.println("Bottone premuto → leggo colore");
        eseguiLetturaColore();
        digitalWrite(LED_BUILTIN, LOW);
      }
    }
  }
}

// --- DEFINIZIONE della funzione (dopo il loop va bene perché c'è il prototipo sopra) ---
void eseguiLetturaColore() {
  lcd.clear();
  lcd.print("Lettura colore...");

  uint16_t r, g, b, c;
  tcs.getRawData(&r, &g, &b, &c);

  // Normalizzazione su 0..255 (solo interi)
  uint32_t maxc = r; if (g > maxc) maxc = g; if (b > maxc) maxc = b;
  if (maxc == 0) maxc = 1;
  uint16_t R = clamp255u((uint32_t)r * 255u / maxc);
  uint16_t G = clamp255u((uint32_t)g * 255u / maxc);
  uint16_t B = clamp255u((uint32_t)b * 255u / maxc);

  Serial.print("RGB raw: "); Serial.print(r); Serial.print(","); Serial.print(g); Serial.print(","); Serial.println(b);
  Serial.print("RGB norm: "); Serial.print(R); Serial.print(","); Serial.print(G); Serial.print(","); Serial.println(B);

  // Usa la tua funzione nel file .h
  String nome = trovaColore(R, G, B);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Colore:");
  lcd.setCursor(0, 1);
  if (nome.length() <= 16) {
    lcd.print(nome);
  } else {
    lcd.print(nome.substring(0, 16)); // evita delay di scrolling
  }
}
