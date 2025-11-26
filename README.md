# HueMate - Color Recognizer per Daltonici

**Arduino + TCS34725 + LCD1602**  

Dispositivo che riconosce i colori di un indumento e mostra il nome sul display.  
L’obiettivo è aiutare persone daltoniche a identificare i colori in modo semplice.

---

## Funzionalità

- Legge un colore tramite sensore RGB **TCS34725**
- Converte i valori letti in RGB normalizzati (0–255)
- Calcola il colore più vicino usando classificazione **HSV/HSL**
- Mostra il nome del colore sul display **LCD 1602**
- Pensato per essere migliorato con:
  - pulsante di lettura
  - sintesi vocale
  - tabella colori personalizzata per indumenti

---

## Componenti utilizzati

- **Arduino UNO / Elegoo UNO R3**
- **TCS34725** sensore RGB (I2C)
- **LCD 1602** con pin paralleli
- **Breadboard** e cavetti jumper
- **Potenziometro 10kΩ** (per il contrasto del display)
- **Bottone** per l'avvio della scansione

---

## Collegamenti Hardware

### TCS34725 → Arduino
| TCS34725 | Arduino |
|----------|---------|
| VIN      | 5V      |
| GND      | GND     |
| SDA      | A4      |
| SCL      | A5      |

---

### LCD 1602 (senza I2C) → Arduino
| LCD Pin | Significato | Arduino |
|--------|-------------|---------|
| 1 (VSS) | GND         | GND     |
| 2 (VDD) | +5V         | 5V      |
| 3 (VO)  | contrasto   | Centrale potenziometro |
| 4 (RS)  | Register Sel. | D7 |
| 5 (RW)  | Read/Write  | **GND fisso** |
| 6 (E)   | Enable      | D8 |
| 11 (D4) | Data        | D9 |
| 12 (D5) | Data        | D10 |
| 13 (D6) | Data        | D11 |
| 14 (D7) | Data        | D12 |
| 15 (A)  | Backlight + | 5V |
| 16 (K)  | Backlight - | GND |

---

## Librerie richieste

Installa da **Arduino IDE → Sketch → Include Library → Manage Libraries…**

- `Adafruit TCS34725`
- `LiquidCrystal` (già inclusa)

---

## Codice principale

Nel progetto è incluso anche il codice commentato.

---

## Autore

Marzia De Maina   
Studentessa di Informatica LM-18  
Università di Bologna
