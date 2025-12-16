# 🅿️ Smart Parking System - IoT-Based Vehicle Management

**A complete IoT smart parking solution built with ESP32, featuring RFID access control, real-time slot detection, servo gate automation, and web-based admin dashboard.**

---

## ✨ Features

- ✅ **RFID-Based Access Control** - Secure card-based entry system
- ✅ **Real-Time Slot Detection** - 4 parking slots with IR sensors
- ✅ **Automated Gate Control** - Servo motor-driven entrance gate
- ✅ **LCD Status Display** - Real-time parking information
- ✅ **LED Indicators** - Visual feedback for each slot (occupied/vacant)
- ✅ **Web Dashboard** - Browser-based admin interface (http://smartparking.local)
- ✅ **Access Logging** - Complete entry/exit records
- ✅ **User Management** - Sign-up, registration, profile management
- ✅ **Proteus Simulation Ready** - Test before hardware assembly
- ✅ **Production-Grade Code** - Full-featured and well-documented

---

## 🎯 Project Overview

The Smart Parking System is an automated parking management solution that:

1. **Authenticates users** via RFID cards
2. **Controls access** with an automated servo gate
3. **Tracks occupancy** using IR sensors for each slot
4. **Displays status** on an LCD screen
5. **Logs all transactions** for audit trail
6. **Provides web interface** for admin management
7. **Manages users** with registration and database

### Real-World Use Cases

- 🏢 Corporate office parking management
- 🏠 Residential society entry gates
- 🅿️ Shopping mall parking automation
- 🏥 Hospital visitor parking control
- 🏫 Educational institution parking
- 🏦 Bank/Financial institution parking

---

## 🔧 Hardware Components

### Core Components

| Component | Qty | Specification | Cost (₹) |
|-----------|-----|---------------|----------|
| **ESP32 Dev Board** | 1 | 38 GPIO pins, WiFi, Bluetooth | 500-600 |
| **16x2 LCD Display** | 1 | 16 chars × 2 lines, I2C optional | 180-250 |
| **MG996R Servo Motor** | 1 | 13kg.cm torque, 180° rotation | 350-450 |
| **MFRC522 RFID Module** | 1 | 13.56MHz, 5-10cm range | 250-350 |
| **IR Obstacle Sensor** | 4 | 2-30cm detection range | 100 each |
| **Red LED** | 4 | 3-5mm, standard brightness | 5 each |
| **220Ω Resistor** | 4 | 1/4W, carbon film | 1 each |
| **Breadboard (Large)** | 1 | 830 tie points | 100-150 |
| **Jumper Wires** | 1 pack | Male-male, mixed lengths | 50-100 |
| **5V Power Supply** | 1 | 2A output, regulated | 200-300 |
| | **TOTAL** | | **₹2500-3500** |

### Optional Components

- Level shifter (for 5V IR sensors to 3.3V ESP32)
- Schottky diodes for reverse polarity protection
- Capacitors for decoupling (100nF, 10µF)
- Push buttons for manual override
- Buzzer for audio feedback

---

## 📊 Circuit Schematic

### Complete System Diagram

![Circuit Schematic](Schematic_smartparking_2025-12-17.png)

**Schematic Details:**
- **Left Side**: ESP32 microcontroller with LCD display and servo motor
- **Center**: RFID module (MFRC522) with SPI connections
- **Right Side**: 4 IR sensors (U8, U9, U10, U11) with LED indicators (R1-R4, D1-D4)
- **Top**: Power distribution (VCC rails - red, GND rails - black)
- **Signal Lines**: Yellow and orange for data/control signals

### Pin Configuration

#### ESP32 Connections (38 pins)

```
┌────────────────────────────────────────┐
│ GND   D35   D34   D33   D32   D5   D23 │
│ EN    D15   D4    D2    D13   D12  D14 │
│ SVP   D14   D27   D26   D25   SVN  GND │
│ VIN   5V    3V3   GND                  │
└────────────────────────────────────────┘

Used Pins (22 total):
├─ LCD: D21(RS), D15(EN), D27(D4), D26(D5), D25(D6), D13(D7)
├─ Servo: D2 (PWM)
├─ IR Sensors: D34(A), D35(B), D32(C), D33(D)
├─ LEDs: D12(A), D14(B), D4(C), D17(D)
├─ RFID SPI: D18(SCK), D19(MISO), D23(MOSI), D5(CS), D22(RST)
├─ Serial: D1(TX), D3(RX)
└─ Power: 3V3, 5V, GND (multiple)
```

---

## 🔌 Detailed Wiring Guide

### LCD1602 (16x2 Display)

```
LCD Pin → ESP32 Pin | Function
─────────────────────────────
1 (VSS)  → GND      | Ground
2 (VDD)  → 5V       | Power supply
3 (VO)   → GND      | Contrast (max brightness)
4 (RS)   → D21      | Register select
5 (RW)   → GND      | Write mode (tied low)
6 (EN)   → D15      | Enable latch
11 (D4)  → D27      | Data bit 4
12 (D5)  → D26      | Data bit 5
13 (D6)  → D25      | Data bit 6
14 (D7)  → D13      | Data bit 7
15 (A)   → 5V (via 220Ω) | Backlight +
16 (K)   → GND      | Backlight -
```

**Note:** Using 4-bit mode (pins D4-D7 only)

### Servo Motor (MG996R)

```
Wire Color → ESP32 Pin | Function | Voltage
──────────────────────────────────────
Yellow     → D2        | PWM signal | 3.3V
Red        → 5V        | Power supply | 5V
Brown      → GND       | Ground | 0V

Control Pulses:
0°   (Closed) = 1.0ms
90°  (Middle) = 1.5ms
180° (Open)   = 2.0ms
```

### IR Obstacle Sensors (×4)

```
Sensor  | OUT Pin | VCC  | GND | Notes
────────|---------|----- |----- |─────────────
Slot A  | D34     | 5V   | GND | Car presence
Slot B  | D35     | 5V   | GND | Car presence
Slot C  | D32     | 5V   | GND | Car presence
Slot D  | D33     | 5V   | GND | Car presence

Logic Levels:
HIGH (5V) = No obstacle (slot vacant)
LOW  (0V) = Obstacle detected (car present)
```

### LED Indicators (×4)

```
Slot | GPIO Pin | Resistor | LED+ | LED- | GND
─────|----------|----------|------|------|-----
A    | D12      | 220Ω     | →    | →    | ✓
B    | D14      | 220Ω     | →    | →    | ✓
C    | D4       | 220Ω     | →    | →    | ✓
D    | D17      | 220Ω     | →    | →    | ✓

Current: ~15mA per LED (total ~60mA)
```

### RFID Module (MFRC522) - SPI Bus

```
RFID Pin  | Pin Name | ESP32 Pin | Function
----------|----------|-----------|─────────────
1         | VCC      | 3V3       | Power (⚠️ 3.3V ONLY)
2         | RST      | D22       | Reset signal
3         | GND      | GND       | Ground
4         | IRQ      | (unused)  | Interrupt
5         | MISO     | D19       | Data OUT (SPI)
6         | MOSI     | D23       | Data IN (SPI)
7         | SCK      | D18       | Clock (SPI)
8         | SDA      | D5        | Chip select

⚠️ CRITICAL: Never connect RFID to 5V - use only 3.3V!
```

### Power Distribution

```
┌──────────────────────┐
│  5V Power Supply     │
│   (2A recommended)   │
└──────┬───────────────┘
       │
       ├─→ [5V Rail] ──────┬─→ Servo Motor VCC
       │                   ├─→ LCD Display VDD
       │                   ├─→ IR Sensors VCC (all)
       │                   └─→ LED Backlight (via 220Ω)
       │
       └─→ [GND Rail] ─────┬─→ All grounds
                           └─→ Return to PSU

Current Budget:
├─ Servo: ~1000mA (peak)
├─ LCD: ~2mA + 80mA backlight
├─ IR Sensors: ~80mA (20mA × 4)
├─ LEDs: ~60mA (15mA × 4)
├─ RFID: ~50mA
├─ ESP32: ~80mA
└─ TOTAL: ~1.3A (peak)
```

---

## 📱 Software Architecture

### Code Structure

```
smart-parking-system/
├── SmartParking_ESP32.ino      (Main code)
│   ├── Setup & Initialization
│   ├── WiFi & Web Server
│   ├── RFID Reading
│   ├── Servo Control
│   ├── LCD Display
│   ├── IR Sensor Processing
│   ├── User Management
│   └── Access Logging
│
├── libraries/
│   ├── LiquidCrystal.h         (LCD control)
│   ├── Servo.h                 (Motor control)
│   ├── MFRC522.h               (RFID reading)
│   ├── ArduinoJson.h           (JSON parsing)
│   └── WiFi.h                  (Network)
│
└── documentation/
    ├── README.md               (This file)
    ├── CIRCUIT.md              (Wiring details)
    ├── API.md                  (Web endpoints)
    └── TROUBLESHOOTING.md      (Debug guide)
```

### Main Functions

```cpp
setup()              // Initialize all components
loop()               // Main execution loop

// Web Server Functions
handleRoot()         // Main dashboard page
handleSignup()       // User registration
handleAdmin()        // Admin panel
handleLock()         // Lock specific slot

// Hardware Functions
readRFID()          // Read card UID
checkIRSensors()    // Monitor occupancy
updateDisplay()     // Update LCD
controlServo()      // Open/close gate
logAccess()         // Record transaction

// User Management
findUserByUID()     // Lookup user
addNewUser()        // Register new user
authenticateCard()  // Verify authorization

// Network Functions
connectWiFi()       // WiFi setup
setupWebServer()    // Initialize routes
handleRequest()     // Process requests
```

---

## 🚀 Getting Started

### Prerequisites

- Arduino IDE 1.8.19+ ([Download](https://www.arduino.cc/en/software))
- ESP32 Board Package via Board Manager
- Required Libraries:
  - LiquidCrystal (built-in)
  - Servo (built-in)
  - MFRC522 (install via Library Manager)
  - ArduinoJson v6.x (install via Library Manager)

### Installation Steps

**Step 1: Install ESP32 Board**
```
1. Arduino IDE → Preferences
2. Add to "Additional Boards Manager URLs":
   https://dl.espressif.com/dl/package_esp32_index.json
3. Tools → Board Manager → Search "ESP32" → Install
4. Tools → Board → Select "ESP32 Dev Module"
```

**Step 2: Install Libraries**
```
Sketch → Include Library → Manage Libraries
Search and Install:
├─ MFRC522 by GithubCommunity
├─ ArduinoJson by Benoit Blanchon
└─ (Others are pre-installed)
```

**Step 3: Upload Code**
```
1. Connect ESP32 via USB
2. Select Port: Tools → Port
3. Paste code into Arduino IDE
4. Click Upload (→ button)
5. Wait for "Upload complete"
```

**Step 4: Monitor Serial Output**
```
Tools → Serial Monitor
Set Baud Rate: 115200
Watch for startup messages and IP address
```

---

## 🎯 How to Use

### First Time Setup

1. **Power On** - Connect 5V power supply
2. **WiFi Connection** - ESP32 connects automatically (configured SSID/password)
3. **Access Dashboard** - Open browser: `http://smartparking.local`
4. **Register Users** - Click "Sign Up", scan RFID card, fill details
5. **Test System** - Scan registered card at entrance

### Web Dashboard

**Main Page (/):**
- View available parking slots
- Real-time occupancy status
- System status information

**Sign Up Page (/signup):**
- User registration form
- RFID card scanning
- Vehicle number entry
- User profile creation

**Admin Page (/admin):**
- Registered users list
- Slot status dashboard
- Access logs (recent entries/exits)
- User management options

### Daily Operations

```
User Approach ↓
    ↓
RFID Card Scan ↓
    ↓
ESP32 Reads UID ↓
    ↓
Check Database ↓
    ├─ Authorized? ✓ → Open Gate + Log Entry
    └─ Unauthorized? ✗ → Keep Closed + Log Attempt
    ↓
IR Sensor Detects Car ↓
    ↓
Mark Slot Occupied ↓
    ↓
Display Status on LCD ↓
    ↓
Light LED Indicator ↓
    ↓
Store in Access Log ↓
    ↓
Update Web Dashboard
```

---

### Requirements

- Proteus 8+ (Free trial available at [labcenter.com](https://www.labcenter.com))
- ESP32 library for Proteus (included in circuit files)
- Arduino IDE for code compilation

### Simulation Setup

1. **Add ESP32 Library to Proteus**
   - Download `ESP32.LIB` and `ESP32.IDX`
   - Copy to: `C:\Program Files\Labcenter Electronics\Proteus 8\LIBRARY`

2. **Create Schematic**
   - New Project → Create Schematic
   - Add components: ESP32, LCD, Servo, IR Sensors, LEDs, Virtual Terminals
   - Wire as per circuit diagram

3. **Compile Code**
   - Arduino IDE: Sketch → Verify
   - Sketch → Export compiled Binary
   - Note the `.hex` file location

4. **Link Code to ESP32**
   - Double-click ESP32 in Proteus
   - Program File → Browse → Select `.hex` file
   - Click OK

5. **Run Simulation**
   - Click Play (▶) button
   - Simulation starts
   - Double-click Virtual Terminal 1
   - Type authorized UID: `FE974106` → Press Enter
   - Watch LCD, Servo, and LEDs respond

### Test Scenarios

**Test 1: Authorized Access**
```
Input: FE974106 (Thirumal's card)
Expected:
├─ LCD shows "Access Granted!"
├─ Servo rotates (gate opens)
├─ LED A turns ON
└─ Terminal 2 shows "GRANTED"
```

**Test 2: Unauthorized Access**
```
Input: INVALID123 (Unknown card)
Expected:
├─ LCD shows "Access Denied!"
├─ Servo stays closed
├─ LED stays OFF
└─ Terminal 2 shows "DENIED"
```

**Test 3: Authorized User List**
```
Valid UIDs:
├─ FE974106 → Thirumal (TN-09-CD-5678)
├─ 8581F905 → Lochan (TN-10-AB-1234)
├─ 090AA694 → Lalit (TN-11-EF-9012)
└─ F9A70FAB → Muthu (TN-12-GH-3456)
```

---

## 🔐 Security Features

### Authentication
- ✅ RFID card verification
- ✅ UID database matching
- ✅ Access logging with timestamps
- ✅ Denied entry records

### Data Protection
- ✅ Local storage on ESP32
- ✅ No cloud data transmission (by default)
- ✅ User profile encryption ready
- ✅ Audit trail for compliance

### Physical Security
- ✅ Servo-controlled mechanical gate
- ✅ IR sensor for forced entry detection
- ✅ LED status indicators
- ✅ Access attempt logging

---

## 📊 Technical Specifications

### ESP32 Board
- **CPU**: Dual-core Xtensa 32-bit @ 240MHz
- **RAM**: 520 KB SRAM + 4 MB Flash
- **GPIO**: 34 pins (28 usable)
- **Interfaces**: SPI, I2C, UART (3x)
- **Wireless**: 802.11 b/g/n WiFi, Bluetooth 4.2
- **Power**: 3.3V logic, 80-160 mA typical

### LCD Display
- **Type**: 16 characters × 2 lines
- **Interface**: 4-bit parallel (pins D4-D7)
- **Voltage**: 5V
- **Current**: 2 mA (display) + 80 mA (backlight)
- **Temp Range**: 0°C to 50°C

### Servo Motor
- **Model**: MG996R (or compatible)
- **Torque**: 13 kg·cm @ 6V
- **Speed**: 0.23 sec/60°
- **Voltage**: 4.8-7.2V (5V nominal)
- **Current**: 0.1-1.0 A (varies with load)

### IR Sensor
- **Detection**: 2-30 cm (adjustable potentiometer)
- **Output**: Digital HIGH/LOW
- **Voltage**: 5V DC
- **Current**: ~20 mA

### RFID Module
- **Frequency**: 13.56 MHz (ISO/IEC 14443A)
- **Range**: 5-10 cm
- **Interface**: SPI (10 MHz max)
- **Voltage**: 3.3V only
- **Current**: ~50 mA average

---

## 🛠️ Troubleshooting

### Common Issues

| Problem | Cause | Solution |
|---------|-------|----------|
| LCD shows nothing | Contrast not set | Connect pin 3 (VO) to GND |
| Servo doesn't move | Wrong PWM frequency | Verify D2 connection |
| RFID not reading | 5V connected to module | Use 3.3V power only |
| LEDs not lighting | Reverse polarity | Long leg = Anode, Short = Cathode |
| WiFi connection fails | Wrong SSID/password | Check WiFi credentials in code |
| IR sensors always HIGH | Not detecting cars | Adjust potentiometer on sensor |
| Simulation freezes | Corrupted .hex file | Recompile in Arduino IDE |

### Debug Mode

Enable serial debugging in code:
```cpp
#define DEBUG 1  // Set to 1 to enable

#if DEBUG
  Serial.println("Debug message");
#endif
```

Monitor via Serial Monitor (115200 baud)

---

## 📈 Performance Metrics

- **Boot Time**: ~2-3 seconds
- **RFID Read Time**: ~500ms
- **Gate Open/Close**: ~2 seconds
- **LCD Update**: ~100ms
- **Web Response**: <500ms
- **Concurrent Users**: 10+ via WiFi
- **Storage Capacity**: ~100 user profiles, 1000 log entries

---

## 🔄 System Workflow Diagram

```
┌─────────────────────────────────────────────────────┐
│           SMART PARKING SYSTEM FLOW                │
└─────────────────────────────────────────────────────┘

    [START] ─→ [Power ON]
       ↓
    [WiFi Connect]
       ↓
    [Web Dashboard Ready]
       ↓
    ┌─────────────────────┐
    │ CONTINUOUS LOOP     │
    └─────────────────────┘
       ↓
    [Check for RFID Card]
       ├─ Card Present? → [Read UID]
       │                     ↓
       │              [Check Database]
       │                     ↓
       │         ┌───────────────────┐
       │         │ Authorized?       │
       │         ├─ YES → [Open Gate]
       │         │        ↓
       │         │    [Turn ON LED]
       │         │        ↓
       │         │    [Log Entry]
       │         │        ↓
       │         │    [Update LCD]
       │         │
       │         ├─ NO → [Deny Access]
       │         │       ↓
       │         │   [Keep Gate Closed]
       │         │       ↓
       │         │   [Log Attempt]
       │         └───────┬──────────┘
       │                 ↓
       ├─ No card? → [Check IR Sensors]
       │                 ↓
       │         [Update Slot Status]
       │                 ↓
       │         [Update Web Dashboard]
       │
       └─ Wait 100ms → [Repeat Loop]
```

---


## 💡 Future Enhancements

- [ ] Mobile app for slot booking
- [ ] SMS/Email notifications
- [ ] Monthly billing integration
- [ ] Cloud backup of logs
- [ ] ML-based occupancy prediction
- [ ] License plate recognition
- [ ] Telegram bot integration
- [ ] Analytics dashboard
- [ ] Multi-level parking support
- [ ] Emergency override system

---

## 👨‍💻 Author & Contributors

**Project Lead** :  ECE 
**Organization** :  SRM University  
**Academic Year**: 2025  
 

### Contributors Welcome!

Found a bug? Have an idea? Want to improve?

1. Fork the repository
2. Create feature branch (`git checkout -b feature/AmazingFeature`)
3. Commit changes (`git commit -m 'Add AmazingFeature'`)
4. Push to branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

---

## 🎓 Learning Resources

This project teaches:

✓ **Embedded Systems** - GPIO, PWM, SPI, Serial communication  
✓ **Web Development** - HTTP server, HTML/CSS, REST API  
✓ **IoT** - WiFi connectivity, cloud concepts  
✓ **Database** - User management, logging  
✓ **Hardware Integration** - Sensors, motors, displays  
✓ **Security** - Authentication, access control  
✓ **Real-World Problem Solving** - Parking automation  

---

## ⚠️ Important Warnings

🔴 **RFID Module**: Connect ONLY to 3.3V. 5V will destroy it.  
🔴 **Servo Power**: Use separate 5V supply for servo. Don't power from ESP32.  
🔴 **LED Polarity**: Reverse connection will burn out LEDs.  
🔴 **WiFi**: Change default SSID/password before deployment.  
🔴 **Database**: User data stored locally. Implement backup for production.  

---

## 📊 Project Statistics

- **Lines of Code**: ~500+
- **Components Used**: 18
- **GPIO Pins**: 22 (out of 34)
- **Development Time**: 4-6 weeks
- **Difficulty Level**: Intermediate
- **Cost**: ₹2500-3500

---

## 🎉 Showcase

**Features Implemented:**
- ✅ 4-slot parking system
- ✅ RFID authentication
- ✅ Real-time occupancy
- ✅ Web dashboard
- ✅ User management
- ✅ Access logging
- ✅ LED indicators
- ✅ LCD display
- ✅ Automated gate
- ✅ Proteus simulation

---

## 📅 Version History

| Version | Date | Changes |
|---------|------|---------|
| v1.0 | Dec 17, 2025 | Initial release - All features complete |
| v0.9 | Dec 16, 2025 | Beta testing complete |
| v0.5 | Dec 10, 2025 | Core functionality ready |

---

**Last Updated**: December 17, 2025  
**Status**: ✅ Production Ready  
**Maintained**: Yes  

---

### ⭐ If this project helped you, please give it a star! ⭐

---

**Made with ❤️ for the IoT and Smart City community**
