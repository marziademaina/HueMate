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

// LED RGB SENSOR
#define LED_PIN 4


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

// Funzione AMPLIATA per identificare molti più colori
const char* identifyColor(uint8_t r, uint8_t g, uint8_t b) {
  float h, s, v;
  rgbToHsv(r, g, b, h, s, v);
  
  if (v < 12) return "Nero";
  if (s < 15 && v >= 12 && v < 35) return "Grigio scuro";
  if (s < 15 && v >= 35 && v < 65) return "Grigio";
  if (s < 15 && v >= 65 && v < 85) return "Grigio chiaro";
  if (s < 15 && v >= 85) return "Bianco";
  
  if (v > 70 && s > 15 && s < 40) {
    if (h < 20 || h >= 345) return "Rosa chiaro";
    if (h >= 20 && h < 50) return "Pesca";
    if (h >= 50 && h < 80) return "Crema";
    if (h >= 80 && h < 150) return "Verde chiaro";
    if (h >= 150 && h < 210) return "Azzurro";
    if (h >= 210 && h < 280) return "Blu chiaro";
    if (h >= 280 && h < 320) return "Lilla";
    if (h >= 320 && h < 345) return "Rosa chiaro";
  }
  
  if (v < 35 && s > 30) {
    if (h < 20 || h >= 340) return "Bordeaux";
    if (h >= 20 && h < 45) return "Marrone scuro";
    if (h >= 45 && h < 70) return "Ocra scuro";
    if (h >= 70 && h < 160) return "Verde scuro";
    if (h >= 160 && h < 210) return "Turchese scuro";
    if (h >= 210 && h < 270) return "Blu scuro";
    if (h >= 270 && h < 310) return "Viola scuro";
    if (h >= 310 && h < 340) return "Magenta scuro";
  }
  
  if (h < 15 || h >= 345) {
    if (v > 60) return "Rosso";
    else return "Rosso scuro";
  }
  if (h >= 15 && h < 25) return "Rosso-arancio";
  if (h >= 25 && h < 40) {
    if (v > 60) return "Arancione";
    else return "Arancione scuro";
  }
  if (h >= 40 && h < 60) {
    if (v < 50) return "Marrone";
    else if (s < 50) return "Beige";
    else return "Arancione";
  }
  if (h >= 50 && h < 70) {
    if (v > 60) return "Giallo";
    else return "Giallo scuro";
  }
  if (h >= 70 && h < 85) return "Giallo-verde";
  if (h >= 85 && h < 140) {
    if (v > 60) return "Verde";
    else return "Verde scuro";
  }
  if (h >= 140 && h < 165) return "Verde acqua";
  if (h >= 165 && h < 190) {
    if (v > 60) return "Turchese";
    else return "Turchese scuro";
  }
  if (h >= 190 && h < 210) return "Azzurro";
  if (h >= 210 && h < 250) {
    if (v > 60) return "Blu";
    else return "Blu scuro";
  }
  if (h >= 250 && h < 270) return "Indaco";
  if (h >= 270 && h < 300) {
    if (v > 60) return "Viola";
    else return "Viola scuro";
  }
  if (h >= 300 && h < 330) {
    if (v > 60) return "Magenta";
    else return "Magenta scuro";
  }
  if (h >= 330 && h < 345) {
    if (s > 60) return "Fucsia";
    else return "Rosa";
  }
  
  return "Sconosciuto";
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

  // Disabilito il pin rgb
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  tcs.setInterrupt(true);
      
}

void loop() {
  int buttonState = digitalRead(BUTTON_PIN);
  digitalWrite(LED_PIN, LOW);
  tcs.setInterrupt(true);
  
  // Pulsante premuto
  if (buttonState == LOW) {
    // Segna quando è stato premuto
    if (buttonPressStart == 0) {
      buttonPressStart = millis();
      Serial.println("[DEBUG] Pulsante premuto");
      tcs.setInterrupt(false);
      digitalWrite(LED_PIN, HIGH);
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