# ESP32 Smart Parking System - Complete Pin Configuration Tables

---

## 📌 Table 1: ESP32 All Pin Names & Functions

| Pin # | Pin Name | Type | Function | Used? | For |
|-------|----------|------|----------|-------|-----|
| 1 | GND | Ground | Ground | ✅ | All components |
| 2 | EN | Input | Enable/Reset | ❌ | Not used |
| 3 | SVP | ADC | Sensor Vp | ❌ | Not used |
| 4 | D14 | GPIO | General Purpose | ❌ | Not used (spare) |
| 5 | D27 | GPIO | General Purpose | ✅ | LCD Data D4 |
| 6 | D26 | GPIO | General Purpose | ✅ | LCD Data D5 |
| 7 | D25 | GPIO | General Purpose | ✅ | LCD Data D6 |
| 8 | SVN | ADC | Sensor Vn | ❌ | Not used |
| 9 | VIN | Power | 5V Input | ✅ | 5V Power Supply |
| 10 | GND | Ground | Ground | ✅ | All components |
| 11 | 3V3 | Power | 3.3V Output | ✅ | RFID Module VCC |
| 12 | 5V | Power | 5V Rail | ✅ | Servo, LEDs, LCD, IR sensors |
| 13 | GND | Ground | Ground | ✅ | All components |
| 14 | D23 | GPIO | SPI MOSI | ✅ | RFID MOSI (SPI) |
| 15 | D5 | GPIO | SPI CS | ✅ | RFID SDA (Chip Select) |
| 16 | D32 | GPIO | General Purpose | ✅ | IR Sensor C Output |
| 17 | D33 | GPIO | General Purpose | ✅ | IR Sensor D Output |
| 18 | D34 | GPIO/ADC | General Purpose | ✅ | IR Sensor A Output |
| 19 | D35 | GPIO/ADC | General Purpose | ✅ | IR Sensor B Output |

---

## 📌 Table 2: Used GPIO Pins - Detailed Assignments

| ESP32 Pin | Pin# | Component | Component Pin | Function | Voltage | Wire Color |
|-----------|------|-----------|----------------|----------|---------|-----------|
| **D1** | 10 | UART/Terminal | RX | Serial Transmit Output | 3.3V | Yellow |
| **D2** | 16 | Servo Motor | Signal | PWM Control | 3.3V | Yellow |
| **D3** | 9 | UART/Terminal | TX | Serial Receive Input | 3.3V | Yellow |
| **D4** | 5 | LCD Display | D7 | Data Bit 7 | 3.3V | Yellow |
| **D4** | 5 | LED D | Anode | LED Indicator Slot D | 3.3V | Red |
| **D5** | 15 | RFID (MFRC522) | SDA | Chip Select (SPI) | 3.3V | Orange |
| **D12** | 22 | LED A | Anode | LED Indicator Slot A | 3.3V | Red |
| **D13** | 23 | LCD Display | D7 | Data Bit 7 | 3.3V | Yellow |
| **D14** | 24 | LED B | Anode | LED Indicator Slot B | 3.3V | Red |
| **D15** | 25 | LCD Display | EN | Enable Signal | 3.3V | Yellow |
| **D17** | 27 | LED D | Anode | LED Indicator Slot D | 3.3V | Red |
| **D18** | 30 | RFID (MFRC522) | SCK | Clock (SPI) | 3.3V | Orange |
| **D19** | 31 | RFID (MFRC522) | MISO | Data Output (SPI) | 3.3V | Orange |
| **D21** | 33 | LCD Display | RS | Register Select | 3.3V | Yellow |
| **D22** | 34 | RFID (MFRC522) | RST | Reset Signal | 3.3V | Orange |
| **D23** | 35 | RFID (MFRC522) | MOSI | Data Input (SPI) | 3.3V | Orange |
| **D25** | 7 | LCD Display | D6 | Data Bit 6 | 3.3V | Yellow |
| **D26** | 6 | LCD Display | D5 | Data Bit 5 | 3.3V | Yellow |
| **D27** | 5 | LCD Display | D4 | Data Bit 4 | 3.3V | Yellow |
| **D32** | 16 | IR Sensor C | OUT | Digital Input | 5V | Yellow |
| **D33** | 17 | IR Sensor D | OUT | Digital Input | 5V | Yellow |
| **D34** | 18 | IR Sensor A | OUT | Digital Input | 5V | Yellow |
| **D35** | 19 | IR Sensor B | OUT | Digital Input | 5V | Yellow |

---

## 📌 Table 3: Power Pins - Distribution

| ESP32 Pin | Voltage | Current (Max) | Connected To | Wire Color |
|-----------|---------|---------------|--------------|-----------|
| **3V3** | 3.3V | 500mA | RFID Module VCC, Logic level conversion | Red |
| **5V** | 5V | 500mA (from external) | Servo Motor, LCD Backlight, IR Sensors, LEDs | Red |
| **GND** | 0V | Unlimited | All components return | Black |
| **VIN** | 5-12V | 1A | External 5V Power Supply input | Red |

**⚠️ WARNING:** ESP32 operates at 3.3V logic. Never connect 5V signals directly to GPIO pins!

---

## 📌 Table 4: LCD1602 (16x2 Display) - Complete Pinout

| LCD Pin | Pin Name | ESP32 Pin | Function | Voltage | Notes |
|---------|----------|-----------|----------|---------|-------|
| 1 | VSS | GND | Ground | 0V | Ground return |
| 2 | VDD | 5V | Power Supply | 5V | Positive supply |
| 3 | VO | GND | Contrast | 0V | Set to GND for max brightness |
| 4 | RS | D21 | Register Select | 3.3V | Instruction/Data select |
| 5 | RW | GND | Read/Write | 0V | Tied to GND (write mode only) |
| 6 | EN | D15 | Enable | 3.3V | Latch enable signal |
| 7 | D0 | Not used | Data Bit 0 | - | 8-bit mode only |
| 8 | D1 | Not used | Data Bit 1 | - | 8-bit mode only |
| 9 | D2 | Not used | Data Bit 2 | - | 8-bit mode only |
| 10 | D3 | Not used | Data Bit 3 | - | 8-bit mode only |
| 11 | **D4** | **D27** | **Data Bit 4** | **3.3V** | **4-bit mode** |
| 12 | **D5** | **D26** | **Data Bit 5** | **3.3V** | **4-bit mode** |
| 13 | **D6** | **D25** | **Data Bit 6** | **3.3V** | **4-bit mode** |
| 14 | **D7** | **D13** | **Data Bit 7** | **3.3V** | **4-bit mode** |
| 15 | A | 5V (via 220Ω) | Backlight + | 5V | Through 220Ω resistor |
| 16 | K | GND | Backlight - | 0V | Ground return |

**Wiring Summary:**
```
LCD Pin 1 (VSS)  → GND
LCD Pin 2 (VDD)  → 5V
LCD Pin 3 (VO)   → GND
LCD Pin 4 (RS)   → D21
LCD Pin 5 (RW)   → GND
LCD Pin 6 (EN)   → D15
LCD Pin 11 (D4)  → D27
LCD Pin 12 (D5)  → D26
LCD Pin 13 (D6)  → D25
LCD Pin 14 (D7)  → D13
LCD Pin 15 (A)   → 5V (220Ω)
LCD Pin 16 (K)   → GND
```

---

## 📌 Table 5: Servo Motor - Complete Connection

| Servo Wire | Color | ESP32 Pin | Function | Voltage | Notes |
|-----------|-------|-----------|----------|---------|-------|
| **Signal** | Yellow/Orange | **D2** | **PWM Control** | **3.3V** | **Pulse width 1-2ms** |
| **VCC** | Red | 5V | Power Supply | 5V | 5V nominal, 4.8-7.2V range |
| **GND** | Brown/Black | GND | Ground | 0V | Common return |

**⚠️ CRITICAL:** 
- Use 5V power supply for servo (not 3.3V)
- PWM signal from D2 (3.3V is OK for servo control)
- Keep servo wires away from signal wires

**Servo Control Pulses:**
```
0°   (Closed)  = 1.0ms pulse
90°  (Middle)  = 1.5ms pulse
180° (Open)    = 2.0ms pulse
```

---

## 📌 Table 6: IR Obstacle Sensors (4x Modules) - Complete

| Sensor | OUT Pin → ESP32 | VCC | GND | Detection Range | Notes |
|--------|-----------------|-----|-----|-----------------|-------|
| **Slot A** | **OUT → D34** | 5V | GND | 2-30cm (adjustable) | Detects car in Slot A |
| **Slot B** | **OUT → D35** | 5V | GND | 2-30cm (adjustable) | Detects car in Slot B |
| **Slot C** | **OUT → D32** | 5V | GND | 2-30cm (adjustable) | Detects car in Slot C |
| **Slot D** | **OUT → D33** | 5V | GND | 2-30cm (adjustable) | Detects car in Slot D |

**IR Sensor 3-Pin Connector:**
```
Pin 1: VCC     → 5V
Pin 2: GND     → GND
Pin 3: OUT/DO  → GPIO (D34/D35/D32/D33)

Logic Levels:
HIGH (5V) → No obstacle detected
LOW  (0V) → Obstacle detected (car present)
```

**Wiring for ALL 4 Sensors:**
```
All VCC pins    → 5V Rail
All GND pins    → GND Rail
Sensor A OUT    → D34
Sensor B OUT    → D35
Sensor C OUT    → D32
Sensor D OUT    → D33
```

---

## 📌 Table 7: LED Indicators (4x with Resistors)

| Slot | LED Anode → Resistor → ESP32 | Resistor Value | LED Cathode | Voltage | Current |
|------|------------------------------|-----------------|-------------|---------|---------|
| **A** | LED(+) → **220Ω** → **D12** | 220Ω | LED(-) → GND | 3.3V | ~15mA |
| **B** | LED(+) → **220Ω** → **D14** | 220Ω | LED(-) → GND | 3.3V | ~15mA |
| **C** | LED(+) → **220Ω** → **D4** | 220Ω | LED(-) → GND | 3.3V | ~15mA |
| **D** | LED(+) → **220Ω** → **D17** | 220Ω | LED(-) → GND | 3.3V | ~15mA |

**LED Connection Schematic:**
```
D12/D14/D4/D17 (GPIO)
        ↓
    [220Ω Resistor]
        ↓
    LED Anode (+)
    [Red LED]
    LED Cathode (-)
        ↓
       GND
```

**⚠️ WARNING - LED Polarity:**
- Long leg = Anode (Positive) → Connect to resistor
- Short leg = Cathode (Negative) → Connect to GND
- Reverse polarity = LED won't light and may burn out

---

## 📌 Table 8: RFID Module (MFRC522) - SPI Configuration

| RFID Pin | Pin Name | ESP32 Pin | SPI Function | Voltage | Notes |
|----------|----------|-----------|--------------|---------|-------|
| 1 | VCC | 3V3 | Power Supply | 3.3V | ⚠️ MUST be 3.3V, NOT 5V |
| 2 | RST | D22 | Reset | 3.3V | Active low reset signal |
| 3 | GND | GND | Ground | 0V | Common return |
| 4 | IRQ | Not used | Interrupt | - | Not used in this project |
| 5 | MISO | D19 | Data Out (SPI) | 3.3V | Receive data from RFID |
| 6 | MOSI | D23 | Data In (SPI) | 3.3V | Send data to RFID |
| 7 | SCK | D18 | Clock (SPI) | 3.3V | SPI clock signal |
| 8 | SDA | D5 | Chip Select | 3.3V | Enable/select RFID chip |

**RFID SPI Bus Summary:**
```
RFID Module ← → ESP32
──────────────────────
VCC  → 3V3 (CRITICAL: 3.3V only!)
GND  → GND
RST  → D22
SDA  → D5 (Chip Select)
MOSI → D23 (Data to RFID)
MISO → D19 (Data from RFID)
SCK  → D18 (Clock)
```

**⚠️ CRITICAL WARNINGS:**
1. **DO NOT connect RFID to 5V** - will destroy the module!
2. Only use 3.3V power supply
3. Keep SPI wires short and shielded if possible
4. SPI clock max speed: 10MHz

---

## 📌 Table 9: Virtual Terminals (Proteus Simulation Only)

| Terminal | TX Pin | RX Pin | Function | Baud Rate | Notes |
|----------|--------|--------|----------|-----------|-------|
| **Terminal 1** | - | **D3 (RXD)** | **RFID UID Input** | **9600** | **Simulates RFID card taps** |
| **Terminal 2** | **D1 (TXD)** | - | **System Output/Logs** | **9600** | **Shows access status** |

**Usage in Proteus:**
```
Simulation Terminal 1 (RFID Input):
1. Double-click Virtual Terminal 1 in schematic
2. Type UID: FE974106
3. Press Enter
4. ESP32 processes and responds
5. Result shown in Terminal 2

Simulation Terminal 2 (Output):
1. Shows "GRANTED" or "DENIED"
2. Shows slot occupancy
3. Shows system status messages
4. All at 9600 baud rate
```

---

## 📌 Table 10: Power Distribution Summary

| Power Source | Voltage | Max Current | Devices Powered | Wire Color |
|--------------|---------|-------------|-----------------|-----------|
| **3V3 Rail** | 3.3V | 500mA | RFID Module, Logic level circuits | Red |
| **5V Rail** | 5V | 1A (recommended 2A) | Servo Motor, LCD Display, IR Sensors, LED Backlight | Red |
| **GND Rail** | 0V | Unlimited | All devices return path | Black |
| **External Supply** | 5V DC | 2A recommended | 5V Rail input (via VIN or 5V pin) | Red |

**Total Current Budget:**
```
Servo Motor:     ~1000mA (peak when moving)
LCD Display:     ~2mA (backlight: ~80mA @ 5V)
IR Sensors (4x): ~80mA (20mA each)
LEDs (4x):       ~60mA (15mA each)
RFID Module:     ~50mA
ESP32 Board:     ~80mA

TOTAL:           ~1.3A (peak)
RECOMMENDED PSU: 5V 2A or higher
```

---

## 📌 Table 11: Pin Status Quick Reference

| ESP32 Pin | Status | Component | Used For | Code Variable |
|-----------|--------|-----------|----------|----------------|
| D1 | ✅ Used | UART | Serial TX (Terminal 2 output) | Serial.print() |
| D2 | ✅ Used | Servo | Gate servo PWM signal | gateServo.write() |
| D3 | ✅ Used | UART | Serial RX (Terminal 1 input) | Serial.read() |
| D4 | ✅ Used | LCD / LED D | LCD data & LED indicator D | pinMode(4, OUTPUT) |
| D5 | ✅ Used | LCD / RFID | LCD data & RFID SDA (CS) | SPI.begin() |
| D12 | ✅ Used | LED A | Slot A indicator | digitalWrite(12, HIGH) |
| D13 | ✅ Used | LCD | LCD data D7 | digitalWrite(13, ...) |
| D14 | ✅ Used | LED B | Slot B indicator | digitalWrite(14, HIGH) |
| D15 | ✅ Used | LCD | LCD enable signal | digitalWrite(15, ...) |
| D17 | ✅ Used | LED D | Slot D indicator | digitalWrite(17, HIGH) |
| D18 | ✅ Used | RFID SPI | SPI clock (SCK) | SPI.begin() |
| D19 | ✅ Used | RFID SPI | SPI data out (MISO) | SPI.begin() |
| D21 | ✅ Used | LCD | LCD register select | digitalWrite(21, ...) |
| D22 | ✅ Used | RFID | RFID reset signal | mfrc522.PCD_Init() |
| D23 | ✅ Used | RFID SPI | SPI data in (MOSI) | SPI.begin() |
| D25 | ✅ Used | LCD | LCD data D6 | digitalWrite(25, ...) |
| D26 | ✅ Used | LCD | LCD data D5 | digitalWrite(26, ...) |
| D27 | ✅ Used | LCD | LCD data D4 | digitalWrite(27, ...) |
| D32 | ✅ Used | IR Sensor C | Slot C detection | digitalRead(32) |
| D33 | ✅ Used | IR Sensor D | Slot D detection | digitalRead(33) |
| D34 | ✅ Used | IR Sensor A | Slot A detection | digitalRead(34) |
| D35 | ✅ Used | IR Sensor B | Slot B detection | digitalRead(35) |
| D0, D6, D7, D8, D9, D10, D11, D16, D20, D24, D28, D29, D30, D31 | ❌ Unused | - | Available for future expansion | - |

---

## 📌 Table 12: Assembly Verification Checklist

- [ ] **LCD1602 Connections:**
  - [ ] Pin 1 (VSS) → GND ✓
  - [ ] Pin 2 (VDD) → 5V ✓
  - [ ] Pin 3 (VO) → GND ✓
  - [ ] Pin 4 (RS) → D21 ✓
  - [ ] Pin 5 (RW) → GND ✓
  - [ ] Pin 6 (EN) → D15 ✓
  - [ ] Pin 11 (D4) → D27 ✓
  - [ ] Pin 12 (D5) → D26 ✓
  - [ ] Pin 13 (D6) → D25 ✓
  - [ ] Pin 14 (D7) → D13 ✓
  - [ ] Pin 15 (A) → 5V via 220Ω ✓
  - [ ] Pin 16 (K) → GND ✓

- [ ] **Servo Motor:**
  - [ ] Yellow wire (Signal) → D2 ✓
  - [ ] Red wire (VCC) → 5V ✓
  - [ ] Brown wire (GND) → GND ✓

- [ ] **IR Sensors (4x):**
  - [ ] All VCC → 5V ✓
  - [ ] All GND → GND ✓
  - [ ] Sensor A OUT → D34 ✓
  - [ ] Sensor B OUT → D35 ✓
  - [ ] Sensor C OUT → D32 ✓
  - [ ] Sensor D OUT → D33 ✓

- [ ] **LEDs with Resistors:**
  - [ ] LED A → 220Ω → D12 → GND ✓
  - [ ] LED B → 220Ω → D14 → GND ✓
  - [ ] LED C → 220Ω → D4 → GND ✓
  - [ ] LED D → 220Ω → D17 → GND ✓
  - [ ] All LED polarity correct (long leg to resistor) ✓

- [ ] **RFID Module:**
  - [ ] VCC → 3V3 (⚠️ NOT 5V) ✓
  - [ ] GND → GND ✓
  - [ ] RST → D22 ✓
  - [ ] SDA → D5 ✓
  - [ ] MOSI → D23 ✓
  - [ ] MISO → D19 ✓
  - [ ] SCK → D18 ✓

- [ ] **Power Rails:**
  - [ ] 5V Rail connected to: Servo, LCD, IR sensors, LED backlight ✓
  - [ ] GND Rail connected to all components ✓
  - [ ] Power supply provides 2A minimum ✓

- [ ] **Final Checks:**
  - [ ] No exposed bare wires ✓
  - [ ] No loose connections ✓
  - [ ] All wire colors correct ✓
  - [ ] Polarity checked on all components ✓
  - [ ] Code compiled and .hex file ready ✓

---

## 📌 Common Mistakes & Fixes

| Mistake | Problem | Solution |
|---------|---------|----------|
| RFID to 5V | Module destroyed | Use only 3.3V for RFID VCC |
| LED reversed | Won't light up | Long leg (anode) must be positive side |
| Wrong LCD pins | No display | Double-check D27-D13 connections |
| Servo to 3.3V only | Weak torque | Use 5V power for servo |
| Missing resistors | LEDs burn out | Use 220Ω for each LED |
| Loose connections | Intermittent failures | Crimp or solder all wires |
| Wrong baud rate | Terminal gibberish | Set to 9600 in both code and terminal |
| D2 not PWM | Servo doesn't move | D2 must be connected, not D2 alternative |

---

## 📌 Wire Color Legend

| Color | Function | Typical Voltage |
|-------|----------|-----------------|
| 🔴 **Red** | Power (+) | 3.3V or 5V |
| ⚫ **Black** | Ground (GND) | 0V |
| 🟡 **Yellow** | Signal / Data | 3.3V-5V |
| 🔵 **Blue** | Alternative Signal | 3.3V-5V |
| 🟠 **Orange** | SPI Data (RFID) | 3.3V |
| ⚪ **White** | Alternative Ground | 0V |
| 🟢 **Green** | Alternative Signal | 3.3V-5V |

---

**Last Updated:** December 16, 2025  
**Status:** ✅ Complete & Ready for Assembly

