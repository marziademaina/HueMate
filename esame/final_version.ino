#include <Wire.h>                 // Libreria I2C per comunicare col sensore
#include <Adafruit_TCS34725.h>    // Libreria per TCS34725
#include <LiquidCrystal.h>        // LCD 16x2
#include <string.h>               // strcpy, strcmp

// -------------------- LCD --------------------
LiquidCrystal lcd(12, 11, 10, 9, 8, 7); // Pin LCD: RS, E, D4, D5, D6, D7

// -------------------- Sensore --------------------
// Nota: GAIN_4X = più sensibilità (attenzione a saturazione con LED alto e oggetto vicino)
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_154MS, TCS34725_GAIN_4X);

// -------------------- Pin --------------------
#define LED_PIN        6          // Pin PWM del LED
#define SCAN_BTN_PIN   3          // Pulsante principale (scan / suggerimento)
#define LIGHT_BTN_PIN  2          // Pulsante secondario (cambia intensità LED)

// -------------------- Livelli LED --------------------
const uint8_t LIGHT_LEVELS[] = {0, 20, 40, 60, 80, 100};                         // Livelli PWM disponibili
const uint8_t NUM_LIGHT_LEVELS = sizeof(LIGHT_LEVELS) / sizeof(LIGHT_LEVELS[0]);  // Numero livelli

uint8_t ledLevelIndex = 2;                 // Indice livello (parte da 40)
uint8_t ledBrightness = LIGHT_LEVELS[2];   // PWM corrente

// -------------------- Messaggio LED su LCD --------------------
bool showLedMsg = false;                   // Se true, stiamo mostrando "Luce: ..."
unsigned long ledMsgStart = 0;             // Quando è iniziato il messaggio LED
const unsigned long LED_MSG_DURATION = 2000; // Durata messaggio LED (ms)

// -------------------- Debounce / press handling --------------------
bool lastLightBtnState = HIGH;             // Stato precedente bottone luce (pullup: HIGH = non premuto)
unsigned long lastLightPressMs = 0;        // Ultima pressione bottone luce (per debounce)

bool scanBtnDown = false;                  // True se bottone scan è attualmente premuto
unsigned long scanBtnDownMs = 0;           // Timestamp di inizio pressione bottone scan
bool longPressActionDone = false;          // True se abbiamo già eseguito l'azione long-press durante questa pressione

const unsigned long DEBOUNCE_MS = 200;     // Debounce semplice
const unsigned long SUGGEST_PRESS_MS = 900; // Dopo 0.9s di pressione: suggerimento abbinamento

// -------------------- Memoria ultimo colore --------------------
bool hasLastColor = false;                 // True se abbiamo una scansione valida memorizzata
char lastBase[12] = "";                    // Colore base: "ROSSO", "GIALLO", ...
char lastTone[8]  = "";                    // Tonalità: "" / "Chiaro" / "Scuro"
char lastScreenLine1[17] = "";
char lastScreenLine2[17] = "";
bool hasLastScreen = false;

// -------------------- Prototipi --------------------
void showHome();                                                   // Schermata home
void showLedLevelOnLCD();                                          // Mostra livello LED
void mostraRisultato(const char* line1, const char* line2);        // Stampa 2 righe su LCD
void readColorHSVBase(char* outBase, char* outTone, bool* ok);     // Scansione colore (HSV base)
void suggestMatch();                                               // Suggerisce colore abbinato

void setup() {
  Serial.begin(9600);                                              // Avvia seriale per debug
  lcd.begin(16, 2);                                                // Inizializza LCD

  pinMode(SCAN_BTN_PIN, INPUT_PULLUP);                             // Bottone scan con pullup
  pinMode(LIGHT_BTN_PIN, INPUT_PULLUP);                            // Bottone luce con pullup
  pinMode(LED_PIN, OUTPUT);                                        // LED come output

  analogWrite(LED_PIN, 0);                                         // LED spento

  if (!tcs.begin()) {                                              // Inizializza sensore
    lcd.clear();                                                   // Pulisce LCD
    lcd.print("Errore Sensore");                                   // Messaggio errore
    while (1) { }                                                  // Blocca se sensore non trovato
  }

  lcd.clear();                                                     // Messaggio iniziale
  lcd.setCursor(0, 0);                                             // Riga 1
  lcd.print("Benvenuti in");
  lcd.setCursor(0, 1);                                             // Riga 2
  lcd.print("HueMate!");
  delay(1500);                                                     // Pausa

  showHome();                                                      // Schermata home

  Serial.println("Sistema pronto.");                               // Debug
  Serial.println("Click breve BTN3: scan");                        // Debug
  Serial.println("Press lungo BTN3: abbina");                      // Debug
  Serial.println("BTN2: livello luce");                            // Debug
}

void loop() {
  // -------------------- 1) Gestione pulsante LUCE (BTN2) --------------------
  bool lightBtnState = digitalRead(LIGHT_BTN_PIN);                 // Legge stato bottone luce

  if (lastLightBtnState == HIGH && lightBtnState == LOW) {         // Fronte di discesa = click
    unsigned long now = millis();                                  // Timestamp attuale
    if (now - lastLightPressMs > DEBOUNCE_MS) {                    // Debounce
      ledLevelIndex = (ledLevelIndex + 1) % NUM_LIGHT_LEVELS;      // Passa al livello successivo
      ledBrightness = LIGHT_LEVELS[ledLevelIndex];                 // Aggiorna PWM

      showLedLevelOnLCD();                                         // Mostra feedback su LCD

      Serial.print("Intensità luce LED -> ");                      // Debug
      Serial.println(ledBrightness);                               // Debug

      lastLightPressMs = now;                                      // Salva tempo per debounce
    }
  }

  lastLightBtnState = lightBtnState;                               // Aggiorna stato precedente

  // -------------------- 2) Timeout messaggio LUCE --------------------
  if (showLedMsg && (millis() - ledMsgStart > LED_MSG_DURATION)) {
  showLedMsg = false;

  // Ripristina ciò che stavi guardando prima (se esiste),
  // altrimenti torna alla Home
  if (hasLastScreen) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(lastScreenLine1);
    lcd.setCursor(0, 1);
    lcd.print(lastScreenLine2);
  } else {
    showHome();
  }
}

  // -------------------- 3) Gestione pulsante SCAN (BTN3) breve vs lungo --------------------
  bool scanBtnState = digitalRead(SCAN_BTN_PIN);                   // Legge bottone scan

  // --- Pressione iniziata ---
  if (!scanBtnDown && scanBtnState == LOW) {                       // Prima volta che lo vediamo premuto
    scanBtnDown = true;                                            // Segna "premuto"
    scanBtnDownMs = millis();                                      // Salva tempo inizio pressione
    longPressActionDone = false;                                   // Reset: non abbiamo ancora fatto long press
  }

  // --- Durante pressione: se supera soglia, fai suggerimento una sola volta ---
  if (scanBtnDown && scanBtnState == LOW) {                        // Se ancora premuto
    unsigned long held = millis() - scanBtnDownMs;                 // Durata pressione

    if (!longPressActionDone && held >= SUGGEST_PRESS_MS) {        // Se è un long press e non l'abbiamo gestito
      suggestMatch();                                              // Mostra abbinamento
      longPressActionDone = true;                                  // Evita ripetizioni finché non rilasci
    }
  }

  // --- Rilascio: se non era long-press, fai scan ---
  if (scanBtnDown && scanBtnState == HIGH) {                       // Rilascio
    unsigned long held = millis() - scanBtnDownMs;                 // Durata pressione
    scanBtnDown = false;                                           // Reset stato premuto

    // Se NON è stato un long press, allora è un click breve => scan
    if (!longPressActionDone && held > 30) {                       // >30ms per evitare rimbalzi/rumore
      char base[12];                                               // Buffer locale base
      char tone[8];                                                // Buffer locale tonalità
      bool ok = false;                                             // Flag esito

      base[0] = '\0';                                              // Inizializza stringa
      tone[0] = '\0';                                              // Inizializza stringa

      readColorHSVBase(base, tone, &ok);                           // Legge colore

      if (ok) {                                                    // Se lettura valida
        hasLastColor = true;                                       // Memorizza che c'è un colore valido
        strcpy(lastBase, base);                                    // Salva base
        strcpy(lastTone, tone);                                    // Salva tonalità
      } else {                                                     // Se lettura non valida
        hasLastColor = false;                                      // Nessun colore memorizzato
        lastBase[0] = '\0';                                        // Pulisce base
        lastTone[0] = '\0';                                        // Pulisce tone
      }
    }
  }
}

// -------------------- HOME --------------------
void showHome() {
  lcd.clear();                                                     // Pulisce LCD
  lcd.setCursor(0, 0);                                             // Riga 1
  lcd.print("BTN3: Click=Scan");                                   // Istruzioni
  lcd.setCursor(0, 1);                                             // Riga 2
  lcd.print("Hold=Abbinamento");                                   // Istruzioni (pulsante lungo)

  delay(3000);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("BTN2: Intensita'");                                   
  lcd.setCursor(0, 1);                                             
  lcd.print("del Led (0-100)");                                   

  delay(3000);
  lcd.clear();
  lcd.print("Inizia ora!");
}

// -------------------- Messaggio livello luce --------------------
void showLedLevelOnLCD() {
  lcd.clear();                                                     // Pulisce LCD
  lcd.setCursor(0, 0);                                             // Riga 1
  lcd.print("Luce: ");                                             // Etichetta

  if (ledBrightness == 0) lcd.print("OFF");                        // LED spento
  else if (ledBrightness == 20) lcd.print("Bassa");                // Bassa
  else if (ledBrightness == 40) lcd.print("Media")   ;              // Media
  else if (ledBrightness == 60) lcd.print("Media-Alta");           // Media-Alta
  else if (ledBrightness == 80) lcd.print("Alta");
  else lcd.print("Massima");                                          // Alta

  lcd.setCursor(0, 1);                                             // Riga 2
  lcd.print("Intensita': ");
  lcd.print(ledBrightness);                                        // Valore PWM

  ledMsgStart = millis();                                          // Salva tempo inizio messaggio
  showLedMsg = true;                                               // Attiva flag
}

// -------------------- Scansione HSV base --------------------
void readColorHSVBase(char* outBase, char* outTone, bool* ok) {
  // --- Parametri acquisizione ---
  const uint8_t  NUM_SAMPLES = 6;                                  // Campioni per media
  const uint16_t C_MIN_VALID = 80;                                 // Sotto: troppo buio/rumore
  const uint16_t C_MAX_VALID = 3500;                               // Sopra: troppo luce/riflesso

  // --- Parametri classificazione ---
  const float S_NEUTRAL = 25.0f;                                   // Saturazione bassa => neutri
  const float V_BLACK   = 45.0f;                                   // V basso => nero
  const float V_LIGHT   = 180.0f;                                  // Sopra => "Chiaro"
  const float V_DARK    = 110.0f;                                  // Sotto => "Scuro"

  // --- Soglie Clear per bianco/nero (tarabili) ---
  const uint16_t C_BLACK = 70;                                     // Sotto: nero probabile
  const uint16_t C_WHITE = 700;                                    // Sopra: bianco probabile

  uint32_t r_s = 0;                                                // Somma R
  uint32_t g_s = 0;                                                // Somma G
  uint32_t b_s = 0;                                                // Somma B
  uint32_t c_s = 0;                                                // Somma C

  *ok = false;                                                     // Default: non valida
  outBase[0] = '\0';                                               // Pulisce base
  outTone[0] = '\0';                                               // Pulisce tone

  lcd.clear();                                                     // Pulisce LCD
  lcd.setCursor(0, 0);                                             // Riga 1
  lcd.print("Analisi...");                                         // Stato
  lcd.setCursor(0, 1);                                             // Riga 2
  lcd.print("Attendere");                                          // Stato

  analogWrite(LED_PIN, ledBrightness);                             // Accende LED al livello scelto
  delay(200);                                                      // Stabilizzazione

  for (uint8_t i = 0; i < NUM_SAMPLES; i++) {                      // Loop campioni
    uint16_t r, g, b, c;                                           // Temporanei per lettura
    tcs.getRawData(&r, &g, &b, &c);                                // Legge raw
    r_s += r;                                                      // Accumula R
    g_s += g;                                                      // Accumula G
    b_s += b;                                                      // Accumula B
    c_s += c;                                                      // Accumula C
    delay(35);                                                     // Pausa tra campioni
  }

  analogWrite(LED_PIN, 0);                                         // Spegne LED

  uint16_t r_avg = (uint16_t)(r_s / NUM_SAMPLES);                  // Media R
  uint16_t g_avg = (uint16_t)(g_s / NUM_SAMPLES);                  // Media G
  uint16_t b_avg = (uint16_t)(b_s / NUM_SAMPLES);                  // Media B
  uint16_t c_avg = (uint16_t)(c_s / NUM_SAMPLES);                  // Media C

  if (c_avg < C_MIN_VALID) {                                       // Troppo buio
    mostraRisultato("Troppo buio", "Aumenta luce");                // LCD
    Serial.print("LED:"); Serial.print(ledBrightness);             // Debug
    Serial.print(" C:");  Serial.print(c_avg);                     // Debug
    Serial.println(" -> TOO DARK");                                // Debug
    return;                                                        // Esce
  }

  if (c_avg > C_MAX_VALID) {                                       // Troppa luce
    mostraRisultato("Troppa luce", "Riduci luce");                 // LCD
    Serial.print("LED:"); Serial.print(ledBrightness);             // Debug
    Serial.print(" C:");  Serial.print(c_avg);                     // Debug
    Serial.println(" -> TOO BRIGHT");                              // Debug
    return;                                                        // Esce
  }

  // --- Normalizzazione su C in scala 0..255 ---
  float r_n = ((float)r_avg / (float)c_avg) * 255.0f;              // R normalizzato
  float g_n = ((float)g_avg / (float)c_avg) * 255.0f;              // G normalizzato
  float b_n = ((float)b_avg / (float)c_avg) * 255.0f;              // B normalizzato

  if (r_n < 0.0f)   r_n = 0.0f;                                    // Clamp minimo
  if (g_n < 0.0f)   g_n = 0.0f;                                    // Clamp minimo
  if (b_n < 0.0f)   b_n = 0.0f;                                    // Clamp minimo
  if (r_n > 255.0f) r_n = 255.0f;                                  // Clamp massimo
  if (g_n > 255.0f) g_n = 255.0f;                                  // Clamp massimo
  if (b_n > 255.0f) b_n = 255.0f;                                  // Clamp massimo

  // --- HSV ---
  float maxV = max(max(r_n, g_n), b_n);                            // Max canale
  float minV = min(min(r_n, g_n), b_n);                            // Min canale
  float d    = maxV - minV;                                        // Delta

  float h = 0.0f;                                                  // Hue
  float s = 0.0f;                                                  // Saturation
  float v = maxV;                                                  // Value (0..255)

  if (maxV > 0.0f) {                                               // Evita divisione per zero
    s = (d / maxV) * 100.0f;                                       // S in percento

    if (d > 0.0001f) {                                             // Se non è grigio
      if (maxV == r_n) {                                           // Dominante rosso
        h = 60.0f * fmodf(((g_n - b_n) / d), 6.0f);                // Calcolo hue
      } else if (maxV == g_n) {                                    // Dominante verde
        h = 60.0f * (((b_n - r_n) / d) + 2.0f);                    // Calcolo hue
      } else {                                                     // Dominante blu
        h = 60.0f * (((r_n - g_n) / d) + 4.0f);                    // Calcolo hue
      }
      if (h < 0.0f) h += 360.0f;                                   // Porta in 0..360
    }
  }

  // -------------------- Classificazione (pochi colori) --------------------
  // NERO: vero nero solo se buio in V o C molto basso + V non alto (evita falsi neri)
  if (v < V_BLACK || (c_avg < C_BLACK && v < 120.0f)) {
    strcpy(outBase, "NERO");                                       // Base
    outTone[0] = '\0';                                             // No tonalità
    *ok = true;                                                    // Valida
  }
  // NEUTRI: saturazione bassa
  else if (s < S_NEUTRAL) {
    if (c_avg > C_WHITE || v > 210.0f) {                           // Bianco
      strcpy(outBase, "BIANCO");                                   // Base
      outTone[0] = '\0';                                           // No tonalità
    } else {                                                       // Grigio
      strcpy(outBase, "GRIGIO");                                   // Base
      if (v >= V_LIGHT) strcpy(outTone, "Chiaro");                 // Chiaro
      else if (v <= V_DARK) strcpy(outTone, "Scuro");              // Scuro
      else outTone[0] = '\0';
    }
    *ok = true;                                                    // Valida
  }
  // COLORI: confini "umani" (H=38 -> GIALLO)
  else {
    const char* base = "COLORE";                                   // Default

    if (h < 20.0f || h >= 340.0f) base = "ROSSO";                  // Rosso
    else if (h < 35.0f)          base = "ARANCIONE";               // Arancione
    else if (h < 80.0f)          base = "GIALLO";                  // Giallo
    else if (h < 165.0f)         base = "VERDE";                   // Verde
    else if (h < 210.0f)         base = "AZZURRO";                 // Azzurro
    else if (h < 270.0f)         base = "BLU";                     // Blu
    else                         base = "VIOLA";                   // Viola

    strcpy(outBase, base);                                         // Copia base

    if (v >= V_LIGHT) strcpy(outTone, "Chiaro");                   // Chiaro
    else if (v <= V_DARK) strcpy(outTone, "Scuro");                // Scuro
    else outTone[0] = '\0';                                        // Niente "Medio"

    *ok = true;                                                    // Valida
  }

  // -------------------- Output su LCD --------------------
  mostraRisultato(outBase, outTone);                               // Stampa base e tonalità

  // -------------------- Log --------------------
  Serial.print("LED:"); Serial.print(ledBrightness);               // Debug
  Serial.print(" C:");  Serial.print(c_avg);                       // Debug
  Serial.print(" H:");  Serial.print(h, 1);                        // Debug
  Serial.print(" S:");  Serial.print(s, 1);                        // Debug
  Serial.print(" V:");  Serial.print(v, 1);                        // Debug
  Serial.print(" -> "); Serial.print(outBase);                     // Debug
  Serial.print(" | ");  Serial.println(outTone);                   // Debug
}

// -------------------- Suggerimento abbinamento --------------------
void suggestMatch() {
  if (!hasLastColor) {                                             // Se non ho una scansione valida
    mostraRisultato("Scansiona", "prima un colore");               // Messaggio
    return;                                                        // Esce
  }

  const char* match = "NERO/BIANCO";                               // Default

  // Suggerimenti semplici (robusti)
  if      (strcmp(lastBase, "ROSSO") == 0)     match = "BLU o BIANCO";
  else if (strcmp(lastBase, "ARANCIONE") == 0) match = "BIANCO o NERO";
  else if (strcmp(lastBase, "GIALLO") == 0)    match = "BLU";
  else if (strcmp(lastBase, "VERDE") == 0)     match = "BIANCO o ROSA";
  else if (strcmp(lastBase, "AZZURRO") == 0)   match = "BIANCO o ROSA";
  else if (strcmp(lastBase, "BLU") == 0)       match = "BIANCO o MARRONE";
  else if (strcmp(lastBase, "VIOLA") == 0)     match = "GRIGIO o MARRONE";
  else if (strcmp(lastBase, "GRIGIO") == 0)    match = "BIANCO o ROSSO";
  else if (strcmp(lastBase, "BIANCO") == 0)    match = "QUASI TUTTO";
  else if (strcmp(lastBase, "NERO") == 0)      match = "QUASI TUTTO";

  mostraRisultato("Abbinamento: ", match);                               // Mostra suggerimento
}

// -------------------- LCD helper --------------------
void mostraRisultato(const char* line1, const char* line2) {
  // Salva l'ultima schermata "importante" (colore, abbina, messaggi)
  strncpy(lastScreenLine1, line1, 16);
  lastScreenLine1[16] = '\0';
  strncpy(lastScreenLine2, line2, 16);
  lastScreenLine2[16] = '\0';
  hasLastScreen = true;

  // Mostra su LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(lastScreenLine1);
  lcd.setCursor(0, 1);
  lcd.print(lastScreenLine2);
}

