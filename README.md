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

## 🧠 La Logica del Sistema

### Spazio Colore HSV (Hue, Saturation, Value)
Per ovviare ai limiti del modello RGB (estremamente sensibile alle ombre), il software implementa l'algoritmo di **Alvy Ray Smith (1978)**.
- **Hue (H):** identifica il colore puro su una ruota cromatica di 360°.
- **Saturation (S):** permette di distinguere i colori "vivi" dalle tonalità neutre (Nero/Bianco/Grigio).
- **Value (V):** definisce se un colore è "Chiaro" o "Scuro".



### Algoritmo di Abbinamento
La logica di abbinamento integra il gusto estetico con la teoria del colore:
- **Neutri:** per Bianco e Nero il sistema suggerisce "Quasi tutto", fornendo sicurezza immediata all'utente.
- **Colori Saturi:** suggerimenti mirati per evitare contrasti stridenti.

---

## 🛠️ Hardware e Collegamenti

### Componenti utilizzati
- **Microcontrollore:** Elegoo UNO R3 (Arduino Compatible)
- **Sensore:** TCS34725 RGB Color Sensor (I2C)
- **Display:** LCD 1602
- **Input:** 2x Pulsanti tattili (Scan & Calibrazione)

### Schema di collegamento
![Collegamenti](report/media/Simulazione HueMate.png)



---

## 📦 Installazione e Uso

1. **Librerie richieste:**
   Installa tramite Arduino Library Manager:
   - `Adafruit TCS34725`
   - `LiquidCrystal` (Libreria standard)

2. **Caricamento:**
   Apri il file `main.ino` con Arduino IDE e carica lo sketch sulla scheda.

3. **Utilizzo:**
   - Avvicina il sensore al tessuto.
   - Premi il **Pulsante 3** per identificare il colore.
   - Tieni premuto il **Pulsante 3** per ricevere un consiglio di abbinamento.
   - Usa il **Pulsante 2** per regolare l'intensità del LED se l'ambiente è troppo buio o troppo luminoso.

---

## 🔮 Sviluppi Futuri
- [ ] **Integrazione Bluetooth e App Mobile** 
- [ ] **Sintesi Vocale**
- [ ] **Database Espandibile**
- [ ] **Personalizzazione Utente**

---

## 🖋️ Autore
**Marzia De Maina** *Studentessa di Informatica (LM-18)* *Università di Bologna*
