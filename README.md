# 🎨 HueMate - Selettore di colori per daltonici
**HueMate** è un progetto basato su Arduino progettato per supportare le persone con discromatopsia (daltonismo) nell'identificazione accurata dei colori e nella scelta di abbinamenti stilistici armoniosi.

Il dispositivo non si limita a nominare il colore, ma funge da vero e proprio consulente d'immagine portatile.

---

## 🚀 Funzionalità principali
- **Rilevamento Alta Precisione:** utilizza il sensore RGB **TCS34725** per una lettura fedele dello spettro visibile.
- **Conversione Percettiva (HSV):** trasforma i dati RGB in spazio colore HSV per isolare la tonalità dalla luminosità ambientale, garantendo stabilità nelle letture.
- **Guida allo Stile:** suggerisce abbinamenti basati sui canoni della moda contemporanea.
- **Calibrazione Dinamica:** LED integrato con intensità regolabile per compensare diverse condizioni di luce (naturale vs artificiale).

---

## 🧠 La logica del sistema
### Spazio Colore HSV (Hue, Saturation, Value)
Per ovviare ai limiti del modello RGB (estremamente sensibile alle ombre), il software implementa l'algoritmo di **Alvy Ray Smith (1978)**.
- **Hue (H):** identifica il colore puro su una ruota cromatica di 360°.
- **Saturation (S):** permette di distinguere i colori "vivi" dalle tonalità neutre (Nero/Bianco/Grigio).
- **Value (V):** definisce se un colore è "Chiaro" o "Scuro".



### Algoritmo di abbinamento
La logica di abbinamento integra il gusto estetico con la teoria del colore:
- **Neutri:** per Bianco e Nero il sistema suggerisce "Quasi tutto", fornendo sicurezza immediata all'utente.
- **Colori Saturi:** suggerimenti mirati per evitare contrasti stridenti.

---

## 🛠️ Hardware e collegamenti

### Componenti utilizzati
- **Microcontrollore:** Elegoo UNO R3 (Arduino Compatible)
- **Sensore:** TCS34725 RGB Color Sensor (I2C)
- **Display:** LCD 1602
- **Input:** 2x Pulsanti tattili (Scan - Pin D3 & Calibrazione luce led - Pin D2)

### Schema di collegamento

| PIN TCS34725 | UTILIZZO | PIN ARDUINO |
| :--- | :--- | :--- |
| **VIN** | Alimentazione positiva 5V | 5V |
| **GND** | Collegamento a terra | GND |
| **SCL** | Segnale di clock per sincronizzazione I2C | A5 |
| **SDA** | Linea dati per la comunicazione I2C | A4 |
| **LED** | Illuminazione del LED integrato | D6 |

| DISPLAY LCD 16x2 | UTILIZZO | PIN ARDUINO |
| :--- | :--- | :--- |
| **VSS** | Collegamento a terra | GND |
| **VDD** | Alimentazione positiva 5V | 5V | 
| **V0** | Regolazione contrasto | Centrale potenziometro |
| **RS** | Registro selezione (Register Select) | D12 |
| **RW** | Lettura/Scrittura (Read/Write) | GND | 
| **E** | Abilitazione (Enable) | D11 | 
| **D4** | Bus dati bit 4 | D10 |
| **D5**| Bus dati bit 5 | D9 |
| **D6** | Bus dati bit 6 | D8 | 
| **D7** | Bus dati bit 7 | D7 |
| **A** | Anodo retroilluminazione | 5V via resistenza 220 Ω | 
| **K** | Catodo retroilluminazione | GND |

---

## 📦 Installazione e uso

1. **Librerie richieste:**
   Installa tramite Arduino Library Manager:
   - `Wire` (Libreria standard)
   - `Adafruit TCS34725`
   - `LiquidCrystal` (Libreria standard)
   - `string` (Libreria standard)

3. **Caricamento:**
   Apri il file `main.ino` con Arduino IDE e carica lo sketch sulla scheda.

4. **Utilizzo:**
   - Avvicina il sensore al tessuto.
   - Premi il **Pulsante 3** per identificare il colore.
   - Tieni premuto il **Pulsante 3** per ricevere un consiglio di abbinamento.
   - Usa il **Pulsante 2** per regolare l'intensità del LED se l'ambiente è troppo buio o troppo luminoso.

---

## 🔮 Sviluppi futuri
- **Integrazione Bluetooth e App Mobile** 
- **Sintesi Vocale**
- **Database Espandibile**
- **Personalizzazione Utente**

---

## 🖋️ Autore
**Marzia De Maina** *Studentessa di Informatica (LM-18)* *Università di Bologna*

---

## 📝 Licenza del documento
HueMate - Selettore di colori per daltonici © 2025 di Marzia De Maina è autorizzato sotto CC BY-NC-SA 4.0. Per visualizzare una copia di questa licenza, visita https://creativecommons.org/licenses/by-nc-sa/4.0/


