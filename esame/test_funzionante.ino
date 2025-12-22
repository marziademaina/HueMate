#include <Wire.h>
#include <Adafruit_TCS34725.h>
#include <LiquidCrystal.h>
#include <avr/sleep.h>
#include <avr/power.h>

LiquidCrystal lcd(7, 8, 9, 10, 11, 12);
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_154MS, TCS34725_GAIN_1X);

#define LED_PIN 6
const int BUTTON_PIN = 2; 

unsigned long buttonPressStart = 0;
const unsigned long LONG_PRESS_TIME = 3000; 

void wakeUp() {
  sleep_disable();
}

void goToSleep() {
  lcd.clear();
  lcd.print("Spegnimento...");
  delay(1000);
  lcd.noDisplay();
  digitalWrite(LED_PIN, LOW);
  tcs.disable();
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), wakeUp, LOW);
  sleep_cpu(); 
  detachInterrupt(digitalPinToInterrupt(BUTTON_PIN));
  tcs.enable();
  lcd.display();
  lcd.clear();
  lcd.print("Sistema Pronto");
  delay(500);
}

void setup() {
  Serial.begin(9600);
  lcd.begin(16, 2);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  if (!tcs.begin()) {
    lcd.print("Errore Sensore");
    while (1);
  }
  lcd.print("Pronto!");
}

void loop() {
  int buttonState = digitalRead(BUTTON_PIN);
  if (buttonState == LOW) {
    if (buttonPressStart == 0) buttonPressStart = millis();
    unsigned long pressDuration = millis() - buttonPressStart;
    if (pressDuration > 1000 && pressDuration < LONG_PRESS_TIME) {
      lcd.setCursor(0, 0);
      lcd.print("Rilascia x Scan");
      lcd.setCursor(0, 1);
      lcd.print("Tieni x Sleep  ");
    }
    if (pressDuration >= LONG_PRESS_TIME) {
      goToSleep();
      buttonPressStart = 0;
    }
  } 
  else {
    if (buttonPressStart > 0) {
      unsigned long pressDuration = millis() - buttonPressStart;
      if (pressDuration < LONG_PRESS_TIME) readColor();
      buttonPressStart = 0;
    }
  }
}

void readColor() {
  uint32_t r_s = 0, g_s = 0, b_s = 0, c_s = 0;
  lcd.clear();
  lcd.print("Lettura...");
  
  digitalWrite(LED_PIN, LOW); 
  delay(250); 

  for(int i=0; i<3; i++) {
    uint16_t r, g, b, c;
    tcs.getRawData(&r, &g, &b, &c);
    r_s += r; g_s += g; b_s += b; c_s += c;
  }
  digitalWrite(LED_PIN, LOW);

  uint16_t c_avg = c_s / 3;
  float r_n = (float)(r_s/3) / c_avg * 255.0;
  float g_n = (float)(g_s/3) / c_avg * 255.0;
  float b_n = (float)(b_s/3) / c_avg * 255.0;

  float h, s, v;
  float maxV = max(max(r_n, g_n), b_n);
  float minV = min(min(r_n, g_n), b_n);
  float d = maxV - minV;
  v = maxV;
  s = (maxV == 0) ? 0 : (d / maxV) * 100.0;

  if (d == 0) h = 0;
  else {
    if (maxV == r_n) h = 60.0 * fmod(((g_n - b_n) / d), 6.0);
    else if (maxV == g_n) h = 60.0 * (((b_n - r_n) / d) + 2.0);
    else h = 60.0 * (((r_n - g_n) / d) + 4.0);
  }
  if (h < 0) h += 360.0;

  const char* colorName = "Sconosciuto";

  if (v < 35) {
      colorName = "Nero";
  } else if (s < 15) {
      if (v > 170) {
          colorName = "Bianco"; 
      } else if (v > 90) {
          colorName = "Grigio Chiaro";
      } else {
          colorName = "Grigio Scuro";
      }
  } else if (h < 20 || h > 340) { // ROSSO e sfumature (h: 0-20 o 340-360)
      if (v < 60) {
          colorName = "Rosso Scuro";
      } else if (v < 100) {
          colorName = s > 60 ? "Rosso" : "Bordeaux";
      } else if (v < 160) {
          if (s > 50) {
              colorName = "Rosso";
          } else {
              colorName = "Rosa Antico";
          }
      } else {
          colorName = s > 40 ? "Rosso Chiaro" : "Rosa";
      }
  } else if (h < 45) { // ARANCIONE e MARRONE (h: 20-45)
      if (v < 60) {
          colorName = "Marrone Scuro";
      } else if (v < 100) {
          if (s < 50) {
              colorName = "Marrone";
          } else {
              colorName = "Arancione Scuro";
          }
      } else if (v < 160) {
          colorName = s > 45 ? "Arancione" : "Beige";
      } else {
          colorName = s > 45 ? "Arancione Chiaro" : "Crema";
      }
  } else if (h < 75) { // GIALLO (h: 45-75)
      if (v < 70) {
          colorName = "Senape";
      } else {
          colorName = "Giallo";
      }
  } else if (h < 160) { // VERDE (h: 75-160)
      if (v < 50) {
          colorName = "Verde Scuro";
      } else if (v < 140) {
          if (s > 45) {
              colorName = "Verde";
          } else {
              colorName = "Verde Chiaro";
          }
      } else {
          colorName = s > 40 ? "Verde Chiaro" : "Verde Menta";
      }
  } else if (h < 210) { // AZZURRO (h: 160-210)
      if (v < 50) {
          colorName = "Azzurro Scuro";
      } else if (v < 150) {
          colorName = "Azzurro";
      } else {
          colorName = s > 40 ? "Azzurro Chiaro" : "Celeste";
      }
  } else if (h < 260) { // BLU (h: 210-260)
      if (v < 50) {
          colorName = "Blu Notte";
      } else if (v < 100) {
          colorName = s > 55 ? "Blu Scuro" : "Blu Grigio";
      } else if (v < 150) {
          colorName = "Blu";
      } else {
          colorName = s > 40 ? "Blu Chiaro" : "Blu Pastello";
      }
  } else if (h < 310) { // VIOLA (h: 260-310)
      if (v < 50) {
          colorName = "Viola Scuro";
      } else if (v < 100) {
          colorName = s > 50 ? "Viola" : "Melanzana";
      } else if (v < 150) {
          colorName = s > 40 ? "Viola" : "Lilla";
      } else {
          colorName = "Lilla Chiaro";
      }
  } else { // ROSA/FUCSIA (h: 310-340)
      if (v < 60) {
          colorName = "Magenta Scuro";
      } else if (v < 110) {
          colorName = "Magenta";
      } else if (v < 160) {
          colorName = s > 45 ? "Fucsia" : "Rosa Intenso";
      } else {
          colorName = s > 40 ? "Fucsia Chiaro" : "Rosa";
      }
  }

  mostraRisultato(colorName);
  Serial.print("H:"); Serial.print(h);
  Serial.print(" S:"); Serial.print(s);
  Serial.print(" V:"); Serial.print(v);
  Serial.print(" R:"); Serial.print(r_n);
  Serial.print(" G:"); Serial.print(g_n);
  Serial.print(" B:"); Serial.print(b_n);
  Serial.print(" -> "); Serial.println(colorName);
}

void mostraRisultato(const char* m) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Colore:");
  lcd.setCursor(0, 1);
  lcd.print(m);
}