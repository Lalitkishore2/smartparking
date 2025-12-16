# Smart Parking System - Circuit Diagram & Documentation

## 📋 Project Overview

This is a **complete IoT-based Smart Parking System** built with ESP32, featuring RFID-based access control, real-time slot management, servo-operated gate control, and web-based admin dashboard.

---

## 🔌 Circuit Components

### Microcontroller
- **ESP32 Dev Board** - Main processing unit
  - 38 GPIO pins
  - Built-in WiFi & Bluetooth
  - Operating Voltage: 3.3V

### Sensors
- **4x IR Obstacle Sensor Modules** - Detect vehicle presence
  - 3-pin modules (VCC, GND, OUT)
  - Operating Voltage: 5V
  - Used for parking slot detection

### Display & Control
- **16x2 LCD Display** - Show parking status
  - 4-bit mode connection
  - Backlight support
  
- **MG996R Servo Motor** - Gate control
  - Operating Voltage: 5V
  - PWM signal control
  - 180° rotation range

### Indicators
- **4x Red LEDs** - Slot status indicators (occupied/vacant)
- **4x 220Ω Resistors** - Current limiting for LEDs

### Communication
- **Virtual Terminal** (Proteus Simulation) - RFID UID input simulation
- **MFRC522 RFID Module** (Real Hardware) - Card reading

### Power Supply
- **5V DC Power Supply** - Powers servo, LEDs, IR sensors, LCD backlight
- **3.3V Rail** - Powers ESP32 (via onboard regulator)

---

## 📐 Circuit Connections

### ESP32 Pin Mapping

```
ESP32 PIN LAYOUT:
┌─────────────────────────────────────┐
│ GND   D35   D34   D33   D32   D5   D23 │
│ EN    D15   D4    D2    D13   D12  D14 │
│ SVP   D14   D27   D26   D25   SVN  GND │
│ VIN   5V    3V3   GND                  │
└─────────────────────────────────────┘
```

### Detailed Pin Connections

#### LCD1602 (16-Pin Display)
```
LCD Pin → ESP32 Pin | Function
─────────────────────────────
1  (VSS)  → GND      | Ground
2  (VDD)  → 5V       | Power
3  (VO)   → GND      | Contrast (tied to GND)
4  (RS)   → D21      | Register Select
5  (RW)   → GND      | Read/Write (write-only)
6  (EN)   → D15      | Enable
11 (D4)   → D27      | Data Bit 4
12 (D5)   → D26      | Data Bit 5
13 (D6)   → D25      | Data Bit 6
14 (D7)   → D13      | Data Bit 7
15 (A)    → 5V       | Backlight+ (through 220Ω)
16 (K)    → GND      | Backlight-
```

#### Servo Motor (MG996R)
```
Servo Wire Color → ESP32 Pin | Function
──────────────────────────────
Yellow (Signal)  → D2         | PWM Control
Red (VCC)        → 5V         | Power
Brown (GND)      → GND        | Ground
```

#### IR Sensors (4x Modules)
```
Slot | OUT Pin → ESP32 | VCC → 5V | GND → GND
─────────────────────────────────────────
A    | OUT → D34       | 5V  | GND
B    | OUT → D35       | 5V  | GND
C    | OUT → D32       | 5V  | GND
D    | OUT → D33       | 5V  | GND
```

#### LEDs with 220Ω Resistors
```
Slot | Connection
──────────────────────────────────────
A    | D12 → [220Ω] → LED(+) ⊕ LED(-) → GND
B    | D14 → [220Ω] → LED(+) ⊕ LED(-) → GND
C    | D4  → [220Ω] → LED(+) ⊕ LED(-) → GND
D    | D17 → [220Ω] → LED(+) ⊕ LED(-) → GND
```

#### RFID Module (Real Hardware - MFRC522)
```
RFID Pin → ESP32 Pin | Function
────────────────────────────
VCC      → 3V3       | Power (3.3V)
GND      → GND       | Ground
RST      → D22       | Reset
SDA      → D5        | Chip Select (SPI)
SCK      → D18       | Clock (SPI)
MOSI     → D23       | Data In (SPI)
MISO     → D19       | Data Out (SPI)
```

#### Virtual Terminal (Proteus Simulation Only)
```
Terminal 1 (RFID Input Simulation):
TX  → D3 (RXD)     | Receive UID data
GND → GND          | Ground

Terminal 2 (System Output/Logs):
RX  → D1 (TXD)     | Send status messages
GND → GND          | Ground
```

---

## 📊 Power Distribution

```
┌─────────────────────────┐
│   5V Power Supply       │
│      (2A recommended)   │
└──────┬──────────────────┘
       │
       ├─→ [5V Rail] ──────┬─→ Servo Motor (VCC)
       │                   ├─→ LCD Display (VDD)
       │                   ├─→ 4x IR Sensors (VCC)
       │                   └─→ LCD Backlight (via 220Ω)
       │
       └─→ [GND Rail] ─────┬─→ All component grounds
                           ├─→ ESP32 GND pins
                           └─→ Return to power supply
```

---

## 🔧 Component Specifications

### ESP32 Dev Board
- **Processor**: Dual-core 32-bit @ 240MHz
- **RAM**: 520 KB SRAM
- **Flash**: 4MB
- **GPIO Pins**: 34 (28 usable)
- **ADC**: 12-bit, 8 channels
- **SPI/I2C/UART**: Multiple interfaces
- **Power**: 3.3V @ 80mA (typical)

### IR Obstacle Sensor
- **Detection Range**: 2-30cm (adjustable)
- **Output**: Digital (HIGH/LOW)
- **Operating Voltage**: 5V DC
- **Current Draw**: ~20mA

### 16x2 LCD Display
- **Resolution**: 16 characters × 2 lines
- **Operating Voltage**: 5V
- **Current Draw**: ~2mA (without backlight)
- **Interface**: 4-bit or 8-bit parallel

### MG996R Servo Motor
- **Operating Voltage**: 4.8-7.2V (5V nominal)
- **Torque**: 13kg.cm at 6V
- **Speed**: 0.23sec/60°
- **Weight**: 55g
- **Control**: PWM (1ms-2ms pulse width)

### MFRC522 RFID Reader
- **Operating Voltage**: 3.3V
- **Communication**: SPI (up to 10MHz)
- **Reading Distance**: 5-10cm
- **Frequency**: 13.56MHz (ISO/IEC 14443A)

---

## ⚡ Wire Color Coding

| Wire Color | Function             | Voltage |
|----------- |----------------------|---------|
| 🔴 Red    | Power (+5V)           | +5V DC  |
| ⚫ Black  | Ground (GND)          | 0V      |
| 🟡 Yellow | Signal/Data (Generic) | 3.3V-5V |
| 🔵 Blue   | Alternative Signal    | 3.3V-5V |
| 🟢 Green  | Alternative Signal    | 3.3V-5V |
| ⚪ White  | Servo Control         | PWM     |

---

## 🎯 Simulation in Proteus

### Required Components (Proteus Names)
```
1. ESP32               - Search: "ESP32"
2. LCD1602             - Search: "LCD" or "LCD1602"
3. Servo Motor         - Search: "SERVO"
4. IR Sensor (×4)      - Search: "IR_SENSOR" or "SENSOR"
5. LED (×4)            - Search: "LED"
6. Resistor 220Ω (×4)  - Search: "RES"
7. Virtual Terminal (×2) - Search: "VIRTUAL" or "TERM"
8. Power Supply        - Search: "PSU" or "POWER"
```

### Simulation Steps
1. ✅ Create new schematic project
2. ✅ Add all components from library
3. ✅ Wire connections as per diagram
4. ✅ Compile code to .hex file (Arduino IDE)
5. ✅ Link .hex to ESP32 in Proteus
6. ✅ Click Play (▶) to start simulation
7. ✅ Open Virtual Terminal 1 for RFID input
8. ✅ Type authorized UID (e.g., FE974106) and press Enter

---

## 🔐 Authorized RFID UIDs (Test Data)

```
FE974106  → Thirumal    (TN-09-CD-5678)
8581F905  → Lochan      (TN-10-AB-1234)
090AA694  → Lalit       (TN-11-EF-9012)
F9A70FAB  → Muthu       (TN-12-GH-3456)
```

---

## 💾 Bill of Materials (BOM)

| # | Component | Quantity | Est. Cost (₹) |
|---|-----------|----------|---------------|
| 1 | ESP32 Dev Board     | 1 | 500-600  |
| 2 | 16x2 LCD Display    | 1 | 180-250  | 
| 3 | MG996R Servo Motor  | 1 | 350-450  | 
| 4 | MFRC522 RFID Module | 1 | 250-350  |
| 5 | IR Sensor Module    | 4 | 100 each | 
| 6 | Red LED             | 4 | 5 each   |
| 7 | 220Ω Resistor       | 4 | 1 each   |
| 8 | Breadboard (Large)  | 1 | 100-150  |
| 9 | Jumper Wires Pack   | 1 | 50-100   |
| 10| 5V Power Supply     | 1 | 200-300  |
| | **TOTAL** |       | **~₹2500-3500**  | 

---

## 📝 Software Requirements

### For Simulation (Proteus)
- ✅ Proteus 8 or higher
- ✅ Arduino IDE (for code compilation)
- ✅ ESP32 Board package (via Arduino IDE)

### For Real Hardware
- ✅ Arduino IDE
- ✅ ESP32 Core library
- ✅ LiquidCrystal library
- ✅ Servo library
- ✅ MFRC522 library
- ✅ WiFi library (for web dashboard)

---

## 🔄 System Workflow

```
START
  ↓
[RFID Card Scanned]
  ↓
[Read UID via MFRC522/VTerminal]
  ↓
[Check Against Database]
  ↓
┌─────────────────────────────┐
│ UID Found?                  │
├─────────────────────────────┤
│ YES → | → Open Gate (Servo) │
│       | → Turn ON LED       │
│       | → Display "Granted" │
│       | → Log Access        │
│       │                     │
│ NO  → | → Servo Stays Closed│
│       | → Turn OFF LED      │
│       | → Display "Denied"  │
│       | → Log Attempt       │
└─────────────────────────────┘
  ↓
[Check IR Sensor for Vehicle]
  ↓
[Mark Slot Occupied/Vacant]
  ↓
[Update LCD Display]
  ↓
[Store in System Log]
  ↓
END

[Repeat every 100ms]
```

---

## 🛠️ Troubleshooting

| Problem                 | Cause                     | Solution                                    | 
|-------------------------|---------------------------|---------------------------------------------|
| LCD not showing text    | Contrast pin (VO) not set | Connect VO to GND or potentiometer          |
| Servo not rotating      | PWM signal issue          | Verify D2 connection to servo signal        |
| LEDs not lighting       | LED polarity reversed     | Long leg = Anode(+), Short leg = Cathode(-) |
| IR sensor not detecting | VCC not connected         | Ensure 5V power on all IR sensors           |
| RFID not reading        | MFRC522 not initialized   | Check SPI pins: SCK(18), MOSI(23), MISO(19) |
| Simulation freezes      | .hex file corrupted       | Recompile code in Arduino IDE               |

---




## 👨‍💻 Author

**Smart Parking System Project**  
ECE|Engineering Student | 
SRM University  
Dec 2025

---

## 📞 Support

For issues or questions:
1. Check the troubleshooting section
2. Review circuit connections
3. Verify code compilation
4. Check Proteus simulation logs

---

**Last Updated**: December 16, 2025  
**Status**: ✅ Proteus Simulation Ready | 🔄 Real Hardware Implementation Pending
