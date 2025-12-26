#include <Wire.h>                 // I2C per il sensore
#include <Adafruit_TCS34725.h>    // Sensore colore TCS34725
#include <LiquidCrystal.h>        // LCD 16x2
#include <string.h>               // strcpy, strcat

// -------------------- LCD --------------------
LiquidCrystal lcd(12, 11, 10, 9, 8, 7);

// -------------------- Sensore --------------------
// Gain 4X: più sensibilità, utile se il segnale è basso (può saturare se LED alto e oggetto vicino)
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_154MS, TCS34725_GAIN_4X);

// -------------------- Pin --------------------
#define LED_PIN        6          // PWM LED
#define SCAN_BTN_PIN   2          // BTN scan
#define LIGHT_BTN_PIN  3          // BTN regolazione luce

// -------------------- Livelli LED --------------------
// 0 = LED spento (utile per test luce ambiente)
const uint8_t LIGHT_LEVELS[] = { 0, 20, 40, 60, 80, 100 };
const uint8_t NUM_LIGHT_LEVELS = sizeof(LIGHT_LEVELS) / sizeof(LIGHT_LEVELS[0]);

uint8_t ledLevelIndex = 2;                 // Parte da 40
uint8_t ledBrightness = LIGHT_LEVELS[2];   // PWM corrente

// -------------------- Messaggio LED su LCD --------------------
bool showLedMsg = false;
unsigned long ledMsgStart = 0;
const unsigned long LED_MSG_DURATION = 2000; // ms

// -------------------- Debounce pulsanti --------------------
bool lastScanBtnState = HIGH;
bool lastLightBtnState = HIGH;
unsigned long lastScanPressMs = 0;
unsigned long lastLightPressMs = 0;
const unsigned long DEBOUNCE_MS = 200;

// -------------------- Prototipi --------------------
void readColor();
void mostraRisultato(const char* line1, const char* line2);
void showLedLevelOnLCD();
void showHome();

void setup() {
  Serial.begin(9600);

  lcd.begin(16, 2);

  pinMode(SCAN_BTN_PIN, INPUT_PULLUP);
  pinMode(LIGHT_BTN_PIN, INPUT_PULLUP);

  pinMode(LED_PIN, OUTPUT);
  analogWrite(LED_PIN, 0);

  if (!tcs.begin()) {
    lcd.clear();
    lcd.print("Errore Sensore");
    while (1) { }
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Rilevatore");
  lcd.setCursor(0, 1);
  lcd.print("Daltonici - OK");
  delay(1500);

  showHome();

  Serial.println("Sistema pronto (daltonici).");
  Serial.print("LED iniziale PWM: ");
  Serial.println(ledBrightness);
}

void loop() {
  // -------- BTN LUCE (cicla livelli) --------
  bool lightBtnState = digitalRead(LIGHT_BTN_PIN);
  if (lastLightBtnState == HIGH && lightBtnState == LOW) {
    unsigned long now = millis();
    if (now - lastLightPressMs > DEBOUNCE_MS) {
      ledLevelIndex = (ledLevelIndex + 1) % NUM_LIGHT_LEVELS;
      ledBrightness = LIGHT_LEVELS[ledLevelIndex];

      showLedLevelOnLCD();
      Serial.print("LED PWM -> ");
      Serial.println(ledBrightness);

      lastLightPressMs = now;
    }
  }
  lastLightBtnState = lightBtnState;

  // -------- Timeout messaggio LED --------
  if (showLedMsg && (millis() - ledMsgStart > LED_MSG_DURATION)) {
    showLedMsg = false;
    showHome();
  }

  // -------- BTN SCAN (lettura colore) --------
  bool scanBtnState = digitalRead(SCAN_BTN_PIN);
  if (lastScanBtnState == HIGH && scanBtnState == LOW) {
    unsigned long now = millis();
    if (now - lastScanPressMs > DEBOUNCE_MS) {
      readColor();
      lastScanPressMs = now;
    }
  }
  lastScanBtnState = scanBtnState;
}

void showHome() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("BTN2=Scan");
  lcd.setCursor(0, 1);
  lcd.print("BTN3=Luce");
}

void showLedLevelOnLCD() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Luce: ");

  if (ledBrightness == 0) lcd.print("OFF");
  else if (ledBrightness <= 30) lcd.print("Bassa");
  else if (ledBrightness <= 60) lcd.print("Media");
  else lcd.print("Alta");

  lcd.setCursor(0, 1);
  lcd.print("PWM:");
  lcd.print(ledBrightness);

  ledMsgStart = millis();
  showLedMsg = true;
}

void readColor() {
  // -------------------- Acquisizione --------------------
  const uint8_t  NUM_SAMPLES = 6;      // più campioni = più stabile
  const uint16_t C_MIN_VALID = 80;     // sotto: segnale scarso (con gain 4X)
  const uint16_t C_MAX_VALID = 3500;   // sopra: probabile riflesso/saturazione (tarabile)

  // -------------------- Classificazione base --------------------
  // Neutri: soglie larghe per evitare che il bianco scappi nei colori
  const float S_NEUTRAL = 25.0f;       // sotto: bianco/grigio/nero (a seconda di C e V)
  const float V_BLACK   = 45.0f;       // nero se V molto basso

  // Tonalità
  const float V_LIGHT = 180.0f;        // chiaro
  const float V_DARK  = 110.0f;        // scuro

  // Soglie su Clear (C) per distinguere bianco/nero meglio del solo HSV
  const uint16_t C_BLACK = 70;         // sotto: nero probabile
  const uint16_t C_WHITE = 700;        // sopra: bianco probabile (con LED medio e gain 4X)

  uint32_t r_s = 0, g_s = 0, b_s = 0, c_s = 0;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Analisi...");
  lcd.setCursor(0, 1);
  lcd.print("Attendere");

  // LED ON al livello scelto
  analogWrite(LED_PIN, ledBrightness);
  delay(200);

  // Letture multiple
  for (uint8_t i = 0; i < NUM_SAMPLES; i++) {
    uint16_t r, g, b, c;
    tcs.getRawData(&r, &g, &b, &c);
    r_s += r; g_s += g; b_s += b; c_s += c;
    delay(35);
  }

  // LED OFF
  analogWrite(LED_PIN, 0);

  uint16_t r_avg = (uint16_t)(r_s / NUM_SAMPLES);
  uint16_t g_avg = (uint16_t)(g_s / NUM_SAMPLES);
  uint16_t b_avg = (uint16_t)(b_s / NUM_SAMPLES);
  uint16_t c_avg = (uint16_t)(c_s / NUM_SAMPLES);

  // Qualità lettura (non provare a “indovinare” se è buio o saturato)
  if (c_avg < C_MIN_VALID) {
    mostraRisultato("Troppo buio", "Aumenta luce");
    Serial.print("LED:"); Serial.print(ledBrightness);
    Serial.print(" C:"); Serial.print(c_avg);
    Serial.println(" -> TOO DARK");
    return;
  }
  if (c_avg > C_MAX_VALID) {
    mostraRisultato("Troppa luce", "Riduci luce");
    Serial.print("LED:"); Serial.print(ledBrightness);
    Serial.print(" C:"); Serial.print(c_avg);
    Serial.println(" -> TOO BRIGHT");
    return;
  }

  // -------------------- Normalizzazione 0..255 --------------------
  float r_n = ((float)r_avg / (float)c_avg) * 255.0f;
  float g_n = ((float)g_avg / (float)c_avg) * 255.0f;
  float b_n = ((float)b_avg / (float)c_avg) * 255.0f;

  if (r_n < 0.0f)   r_n = 0.0f;
  if (g_n < 0.0f)   g_n = 0.0f;
  if (b_n < 0.0f)   b_n = 0.0f;
  if (r_n > 255.0f) r_n = 255.0f;
  if (g_n > 255.0f) g_n = 255.0f;
  if (b_n > 255.0f) b_n = 255.0f;

  // -------------------- HSV --------------------
  float maxV = max(max(r_n, g_n), b_n);
  float minV = min(min(r_n, g_n), b_n);
  float d    = maxV - minV;

  float h = 0.0f;
  float s = 0.0f;
  float v = maxV;  // 0..255

  if (maxV > 0.0f) {
    s = (d / maxV) * 100.0f; // 0..100

    if (d > 0.0001f) {
      if (maxV == r_n) {
        h = 60.0f * fmodf(((g_n - b_n) / d), 6.0f);
      } else if (maxV == g_n) {
        h = 60.0f * (((b_n - r_n) / d) + 2.0f);
      } else {
        h = 60.0f * (((r_n - g_n) / d) + 4.0f);
      }
      if (h < 0.0f) h += 360.0f;
    }
  }

  // -------------------- CLASSIFICAZIONE (POCA, STABILE) --------------------
  // Output su due righe: LINE1 = colore base, LINE2 = Chiaro/Medio/Scuro
  static char line1[17];
  static char line2[17];
  line1[0] = '\0';
  line2[0] = '\0';

  // (A) NERO
  // Nero: solo se davvero buio in V o C (evita falsi neri quando C è solo “un po' basso”)
  if (v < V_BLACK || (c_avg < C_BLACK && v < 120.0f)) {
    strcpy(line1, "NERO");
    strcpy(line2, "");
  }
  // (B) NEUTRI
  else if (s < S_NEUTRAL) {
    if (c_avg > C_WHITE || v > 210.0f) {
      strcpy(line1, "BIANCO");
      strcpy(line2, "");
    } else {
      strcpy(line1, "GRIGIO");
      if (v >= 170.0f) strcpy(line2, "Chiaro");
      else if (v <= 120.0f) strcpy(line2, "Scuro");
      else strcpy(line2, "Medio");
    }
  }
  // (C) COLORI PRINCIPALI (con confini “umani”)
  else {
    const char* base = "COLORE";

    // Nota importante:
    // Qui scegliamo confini più "intuitivi" (user-friendly), non "da manuale HSV".
    // Così H=38 diventa GIALLO (come vuoi tu), non ARANCIONE.

    // ROSSO
    if (h < 20.0f || h >= 340.0f) base = "ROSSO";
    // ARANCIONE (solo arancio "vero", stretto)
    else if (h < 35.0f)          base = "ARANCIONE";
    // GIALLO (include 35..80 -> H=38 finisce qui)
    else if (h < 80.0f)          base = "GIALLO";
    // VERDE
    else if (h < 165.0f)         base = "VERDE";
    // AZZURRO
    else if (h < 210.0f)         base = "AZZURRO";
    // BLU
    else if (h < 270.0f)         base = "BLU";
    // VIOLA
    else                         base = "VIOLA";

    strcpy(line1, base);

    // Tonalità (solo chiaro/medio/scuro)
    if (v >= V_LIGHT) strcpy(line2, "Chiaro");
    else if (v <= V_DARK) strcpy(line2, "Scuro");
    else strcpy(line2, "Medio");
  }

  mostraRisultato(line1, line2);

  // -------------------- Log --------------------
  Serial.print("LED:"); Serial.print(ledBrightness);
  Serial.print(" C:");  Serial.print(c_avg);
  Serial.print(" H:");  Serial.print(h, 1);
  Serial.print(" S:");  Serial.print(s, 1);
  Serial.print(" V:");  Serial.print(v, 1);
  Serial.print(" R:");  Serial.print(r_n, 1);
  Serial.print(" G:");  Serial.print(g_n, 1);
  Serial.print(" B:");  Serial.print(b_n, 1);
  Serial.print(" -> "); Serial.print(line1);
  Serial.print(" | ");  Serial.println(line2);
}

void mostraRisultato(const char* line1, const char* line2) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(line1);
  lcd.setCursor(0, 1);
  lcd.print(line2);
}
