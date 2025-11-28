// Includiamo la libreria Wire per usare il bus I2C
#include <Wire.h>

// Includiamo la libreria ufficiale Adafruit per il sensore colore TCS34725
#include <Adafruit_TCS34725.h>

// Includiamo la libreria per gestire il display LCD 1602
#include <LiquidCrystal.h>

// Libreria per la modalità sleep (risparmio energetico)
#include <avr/sleep.h>
#include <avr/power.h>

// Creiamo l'oggetto lcd indicando i pin collegati
LiquidCrystal lcd(7, 8, 9, 10, 11, 12);

// Creiamo l'oggetto per il sensore TCS34725
Adafruit_TCS34725 tcs = Adafruit_TCS34725(
  TCS34725_INTEGRATIONTIME_50MS,
  TCS34725_GAIN_4X
);

// ===== CONFIGURAZIONE PULSANTE =====
const int BUTTON_PIN = 2;  // DEVE essere pin 2 o 3 per l'interrupt
bool buttonPressed = false;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

// Variabili per gestire la pressione prolungata
unsigned long buttonPressStart = 0;
const unsigned long LONG_PRESS_TIME = 3000;  // 3 secondi

// Variabile per sapere se siamo in modalità sleep
volatile bool wakeUpFlag = false;

// Funzione per convertire RGB (0-255) in HSV
void rgbToHsv(uint8_t r, uint8_t g, uint8_t b, float &h, float &s, float &v) {
  float rf = r / 255.0;
  float gf = g / 255.0;
  float bf = b / 255.0;
  
  float maxVal = max(max(rf, gf), bf);
  float minVal = min(min(rf, gf), bf);
  float delta = maxVal - minVal;
  
  if (delta == 0) {
    h = 0;
  } else if (maxVal == rf) {
    h = 60.0 * fmod(((gf - bf) / delta), 6.0);
  } else if (maxVal == gf) {
    h = 60.0 * (((bf - rf) / delta) + 2.0);
  } else {
    h = 60.0 * (((rf - gf) / delta) + 4.0);
  }
  
  if (h < 0) h += 360.0;
  s = (maxVal == 0) ? 0 : (delta / maxVal) * 100.0;
  v = maxVal * 100.0;
}

// Struttura per definire un range di colore
struct ColorRange {
  const char* name;
  float hMin, hMax;    // Range di Hue (0-360)
  float sMin, sMax;    // Range di Saturation (0-100)
  float vMin, vMax;    // Range di Value/Brightness (0-100)
};

// Dizionario dei colori - ordinato per priorità di controllo
const ColorRange colorDictionary[] = {
  // COLORI ACROMATICI (bassa saturazione)
  {"Nero",           0,   360,  0,  100,  0,   12},
  {"Grigio",         0,   360,  0,   15, 35,   65},
  {"Bianco",         0,   360,  0,   15, 85,  100},
  
  // COLORI PASTELLO (alta luminosità, bassa saturazione)
  {"Rosa chiaro",    320, 360, 15,   40, 70,  100},
  {"Pesca",           20,  50, 15,   40, 70,  100},
  {"Crema",           50,  80, 15,   40, 70,  100},
  {"Azzurro",        150, 210, 15,   40, 70,  100},
  {"Lilla",          280, 320, 15,   40, 70,  100},
  
  // COLORI SCURI (bassa luminosità, alta saturazione)
  {"Bordeaux",       340, 360, 30,  100,  0,   35},
  {"Ocra scuro",      45,  70, 30,  100,  0,   35},
  {"Blu scuro",      210, 270, 30,  100,  0,   35},
  
  // ROSSI
  {"Rosso",          345, 360, 15,  100, 60,  100},
  {"Rosso scuro",    345, 360, 15,  100,  0,   60},
  
  // ARANCIONI E MARRONI
  {"Arancione",       25,  40, 15,  100, 60,  100},
  {"Marrone",         40,  60, 15,  100,  0,   50},
  {"Beige",           40,  60, 15,   50, 50,  100},
  {"Arancione",       40,  50, 50,  100, 50,  100},
  
  // GIALLI
  {"Giallo",          50,  70, 15,  100, 60,  100},
  
  // VERDI
  {"Verde",           85, 140, 15,  100, 60,  100},
  
  // TURCHESI E AZZURRI
  {"Turchese",       165, 190, 15,  100, 60,  100},
  
  // BLU
  {"Blu",            210, 250, 15,  100, 60,  100},
  
  // VIOLA E MAGENTA
  {"Viola",          270, 300, 15,  100, 60,  100},
  {"Magenta",        300, 330, 15,  100, 60,  100},
  
  // ROSA E FUCSIA
  {"Fucsia",         330, 345, 60,  100, 15,  100},
};

const int dictionarySize = sizeof(colorDictionary) / sizeof(ColorRange);

// Funzione che identifica il colore usando il dizionario
const char* identifyColorFromDictionary(float h, float s, float v) {
  // Scorre il dizionario e trova il primo match
  for (int i = 0; i < dictionarySize; i++) {
    const ColorRange& color = colorDictionary[i];
    
    // Gestione speciale per i rossi che attraversano 0°
    bool hueMatch = false;
    if (color.hMin > color.hMax) {
      // Caso speciale: attraversa 0° (es: 345-360 e 0-15)
      hueMatch = (h >= color.hMin || h <= color.hMax);
    } else {
      // Caso normale
      hueMatch = (h >= color.hMin && h <= color.hMax);
    }
    
    // Controlla se tutti i parametri HSV rientrano nel range
    if (hueMatch &&
        s >= color.sMin && s <= color.sMax &&
        v >= color.vMin && v <= color.vMax) {
      return color.name;
    }
  }
  
  return "Sconosciuto";
}

// Funzione wrapper che accetta RGB e restituisce il nome del colore
const char* identifyColor(uint8_t r, uint8_t g, uint8_t b) {
  float h, s, v;
  rgbToHsv(r, g, b, h, s, v);
  return identifyColorFromDictionary(h, s, v);
}

// ESEMPIO: Funzione per stampare tutto il dizionario (utile per debug)
void printColorDictionary() {
  Serial.println("\n=== DIZIONARIO COLORI ===");
  Serial.println("Nome | H(min-max) | S(min-max) | V(min-max)");
  Serial.println("---------------------------------------------");
  
  for (int i = 0; i < dictionarySize; i++) {
    const ColorRange& color = colorDictionary[i];
    Serial.print(color.name);
    Serial.print(" | ");
    Serial.print(color.hMin, 0);
    Serial.print("-");
    Serial.print(color.hMax, 0);
    Serial.print(" | ");
    Serial.print(color.sMin, 0);
    Serial.print("-");
    Serial.print(color.sMax, 0);
    Serial.print(" | ");
    Serial.print(color.vMin, 0);
    Serial.print("-");
    Serial.println(color.vMax, 0);
  }
  
  Serial.print("\nTotale colori definiti: ");
  Serial.println(dictionarySize);
  Serial.println("=========================\n");
}

// Funzione che legge il colore e lo mostra sul display
void readAndDisplayColor() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Lettura...");
  
  uint16_t r_raw, g_raw, b_raw, c_raw;
  tcs.getRawData(&r_raw, &g_raw, &b_raw, &c_raw);

  if (c_raw == 0) c_raw = 1;

  float r_float = (float)r_raw / (float)c_raw * 255.0;
  float g_float = (float)g_raw / (float)c_raw * 255.0;
  float b_float = (float)b_raw / (float)c_raw * 255.0;

  uint8_t r = (uint8_t)constrain((int)r_float, 0, 255);
  uint8_t g = (uint8_t)constrain((int)g_float, 0, 255);
  uint8_t b = (uint8_t)constrain((int)b_float, 0, 255);

  float h, s, v;
  rgbToHsv(r, g, b, h, s, v);

  const char* colorName = identifyColor(r, g, b);

  Serial.print("RAW  R:");
  Serial.print(r_raw);
  Serial.print(" G:");
  Serial.print(g_raw);
  Serial.print(" B:");
  Serial.print(b_raw);
  Serial.print(" C:");
  Serial.println(c_raw);
  
  Serial.print("RGB  R:");
  Serial.print(r);
  Serial.print(" G:");
  Serial.print(g);
  Serial.print(" B:");
  Serial.println(b);
  
  Serial.print("HSV  H:");
  Serial.print(h, 1);
  Serial.print("° S:");
  Serial.print(s, 1);
  Serial.print("% V:");
  Serial.print(v, 1);
  Serial.println("%");
  
  Serial.print("Colore: ");
  Serial.println(colorName);
  Serial.println("-------------------------");

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Colore:");
  lcd.setCursor(0, 1);
  lcd.print(colorName);
}

// Funzione ISR chiamata quando il pulsante viene premuto durante lo sleep
void wakeUp() {
  sleep_disable();
  wakeUpFlag = true;
}

// Funzione per mettere Arduino in sleep mode
void goToSleep() {
  Serial.println("\n>>> Entro in SLEEP MODE <<<");
  Serial.flush();
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Spegnimento");
  lcd.setCursor(0, 1);
  lcd.print("in corso...");
  
  delay(1000);
  lcd.clear();
  
  // Disabilita il sensore
  tcs.disable();
  
  // Configura l'interrupt
  wakeUpFlag = false;
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), wakeUp, LOW);
  
  // Imposta sleep mode
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  
  // VA A DORMIRE
  sleep_cpu();
  
  // ===== RISVEGLIO =====
  
  detachInterrupt(digitalPinToInterrupt(BUTTON_PIN));
  
  Serial.println(">>> RISVEGLIO <<<");
  
  // Aspetta che il pulsante sia rilasciato
  while(digitalRead(BUTTON_PIN) == LOW) {
    delay(10);
  }
  
  delay(500);
  
  // Riattiva il sensore
  tcs.enable();
  delay(100);
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Risveglio!");
  delay(1500);
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Premi pulsante");
  lcd.setCursor(0, 1);
  lcd.print("per leggere");
  
  // Reset variabili
  buttonPressed = false;
  buttonPressStart = 0;
  wakeUpFlag = false;
  
  Serial.println("Sistema pronto. Tieni premuto 3 sec per spegnere.");
}

void setup() {
  Serial.begin(9600);
  lcd.begin(16, 2);
  
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // lcd.setCursor(0, 0);
  // lcd.print("Avvio sensore");
  // lcd.setCursor(0, 1);
  // lcd.print("TCS34725...");

  if (!tcs.begin()) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Errore sensore");
    Serial.println("ERRORE: sensore TCS34725 non trovato.");
    while (1);
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Pronto?! Prendi");
  lcd.setCursor(0, 1);
  lcd.print("i tuoi vestiti...");
  Serial.println("Sensore TCS34725 inizializzato.");
  

  delay(3000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Premi il bottone");
  lcd.setCursor(0, 1);
  lcd.print("ogni volta che");

  delay(2000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("vuoi scansionare");
  lcd.setCursor(0, 1);
  lcd.print("qualcosa!");

  delay(2000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("o tieni premuto");
  lcd.setCursor(0, 1);
  lcd.print("per spegnermi!");

  Serial.println("Sistema pronto.");
  Serial.println("- Premi breve: leggi colore");
  Serial.println("- Tieni premuto 3 sec: SLEEP MODE");
}

void loop() {
  int buttonState = digitalRead(BUTTON_PIN);
  
  // Pulsante premuto
  if (buttonState == LOW) {
    // Segna quando è stato premuto
    if (buttonPressStart == 0) {
      buttonPressStart = millis();
      Serial.println("[DEBUG] Pulsante premuto");
    }
    
    unsigned long pressDuration = millis() - buttonPressStart;
    
    // Mostra feedback ogni secondo
    if (pressDuration >= 1000 && pressDuration < 2000) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Tieni premuto");
      lcd.setCursor(0, 1);
      lcd.print("ancora 2 sec...");
    }
    else if (pressDuration >= 2000 && pressDuration < 3000) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Tieni premuto");
      lcd.setCursor(0, 1);
      lcd.print("ancora 1 sec...");
    }
    
    // Se è premuto da più di 3 secondi
    if (pressDuration >= LONG_PRESS_TIME) {
      Serial.println("[DEBUG] Pressione lunga rilevata - vado in SLEEP");
      goToSleep();
      buttonPressStart = 0; // Reset dopo il risveglio
    }
  } 
  // Pulsante rilasciato
  else {
    // Se era stato premuto ma rilasciato prima di 3 secondi
    if (buttonPressStart > 0) {
      unsigned long pressDuration = millis() - buttonPressStart;
      
      if (pressDuration < LONG_PRESS_TIME) {
        // Pressione breve = leggi colore
        Serial.println("[DEBUG] Pressione breve - leggo colore");
        
        if ((millis() - lastDebounceTime) > debounceDelay) {
          lastDebounceTime = millis();
          readAndDisplayColor();
          Serial.println("Premi di nuovo (tieni 3s per sleep).\n");
        }
      }
      
      buttonPressStart = 0; // Reset
    }
  }
  
  delay(50);
}