/*
 * ESP32 SMART PARKING SYSTEM v5.0 - FIRESTORE EDITION
 * ====================================================
 * Migrated from Firebase Realtime Database to Firestore
 * Matches web app Firestore schema exactly
 * 
 * Features:
 *   - RFID scanning with MFRC522
 *   - IR sensor monitoring (4 slots)
 *   - Servo gate control
 *   - 16x2 LCD display
 *   - Firestore REST API sync
 *   - Real-time slot booking sync
 *   - Activity logging
 *   - Heartbeat monitoring
 * 
 * Firestore Collections:
 *   - parkingSlots/{slot-a, slot-b, slot-c, slot-d}
 *   - activityLogs (auto-generated IDs)
 *   - users (read-only for RFID lookup)
 *   - systemConfig/esp32Status
 *   - pendingRFID (for signup flow)
 */

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <LiquidCrystal.h>
#include <ESP32Servo.h>
#include <SPI.h>
#include <MFRC522.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <time.h>

// ==================== CONFIGURATION ====================
// WiFi Configuration
const char* ssid = "POCO";
const char* password = "lalitkishore27";

// Firebase Firestore Configuration
const char* FIREBASE_PROJECT_ID = "smartparkingsystem-bcea5";
const char* FIREBASE_API_KEY = "AIzaSyBF2zuJAzrVoCOU8eZ-oYJwJsBMlRpq-uE";
const String FIRESTORE_BASE_URL = "https://firestore.googleapis.com/v1/projects/" + 
                                   String(FIREBASE_PROJECT_ID) + 
                                   "/databases/(default)/documents";

// Device Configuration
const char* DEVICE_ID = "ESP32-GATE-001";

// ==================== PIN DEFINITIONS ====================
#define SS_PIN 5
#define RST_PIN 22
#define LCD_RS 21
#define LCD_EN 15
#define LCD_D4 27
#define LCD_D5 26
#define LCD_D6 25
#define LCD_D7 13
#define SERVO_PIN 2

#define IR_SLOT_A 34
#define IR_SLOT_B 35
#define IR_SLOT_C 32
#define IR_SLOT_D 33

#define LED_SLOT_A 12
#define LED_SLOT_B 14
#define LED_SLOT_C 4
#define LED_SLOT_D 17

// ==================== OBJECTS ====================
LiquidCrystal lcd(LCD_RS, LCD_EN, LCD_D4, LCD_D5, LCD_D6, LCD_D7);
MFRC522 mfrc522(SS_PIN, RST_PIN);
Servo gateServo;
Preferences preferences;
WiFiClientSecure wifiClient;

// ==================== DATA STRUCTURES (FIRESTORE COMPATIBLE) ====================
struct CurrentBooking {
  String userId;
  String userName;
  String vehicleNumber;
  String rfidTag;
  String bookedAt;    // ISO timestamp
  String entryTime;   // ISO timestamp
};

struct ParkingSlot {
  String slotId;      // "slot-a", "slot-b", etc.
  String slotName;    // "A", "B", "C", "D"
  int irPin;
  int ledPin;
  bool isAvailable;   // Changed from "occupied" - inverted logic
  CurrentBooking currentBooking;
  String lastUpdated;
  bool lastIRState;
  unsigned long lastIRChange;
  unsigned long localEntryTime;
};

ParkingSlot slots[4] = {
  {"slot-a", "A", IR_SLOT_A, LED_SLOT_A, true, {"", "", "", "", "", ""}, "", false, 0, 0},
  {"slot-b", "B", IR_SLOT_B, LED_SLOT_B, true, {"", "", "", "", "", ""}, "", false, 0, 0},
  {"slot-c", "C", IR_SLOT_C, LED_SLOT_C, true, {"", "", "", "", "", ""}, "", false, 0, 0},
  {"slot-d", "D", IR_SLOT_D, LED_SLOT_D, true, {"", "", "", "", "", ""}, "", false, 0, 0}
};

// ==================== SYSTEM VARIABLES ====================
int availableSlots = 4;
bool gateOpen = false;
bool rfidReaderOK = false;  // Track if RFID reader initialized properly
String lastUser = "None";
String currentRFID = "";
bool rfidDetected = false;
unsigned long rfidDetectedTime = 0;
const unsigned long RFID_COOLDOWN = 3000;  // 3 seconds between scans
int totalEntries = 0;
int totalExits = 0;

// Gate control
unsigned long gateOpenTime = 0;
const unsigned long GATE_OPEN_DURATION = 5000;  // 5 seconds

// Timing variables for Firestore sync
unsigned long lastDisplayUpdate = 0;
unsigned long lastHeartbeat = 0;
unsigned long lastSlotSync = 0;
unsigned long lastBookingCheck = 0;
unsigned long lastGateCommandCheck = 0;  // NEW: Gate control polling
int lastFirestoreGetHttpCode = 0;
unsigned long firestoreBackoffUntil = 0;
unsigned long firestoreBackoffMs = 0;
unsigned long lastFirestoreGetAt = 0;

const unsigned long HEARTBEAT_INTERVAL = 20000;    // 20 seconds
const unsigned long SLOT_SYNC_INTERVAL = 12000;    // 12 seconds (faster booking pickup)
const unsigned long DISPLAY_INTERVAL = 3000;      // 3 seconds
const unsigned long BOOKING_CHECK_INTERVAL = 60000; // 1 minute
const unsigned long GATE_COMMAND_INTERVAL = 30000;   // 30 seconds (read budget balanced)
const unsigned long FIRESTORE_GET_MIN_GAP = 1500;    // avoid back-to-back GET bursts
const unsigned long TIME_SYNC_RETRY_INTERVAL = 60000; // retry NTP every 60 seconds when unsynced
const time_t MIN_VALID_UNIX_TIME = 1704067200;        // 2024-01-01T00:00:00Z
unsigned long lastTimeSyncAttempt = 0;

int displayMode = 0;

// ==================== FUNCTION DECLARATIONS ====================
void initHardware();
void connectToWiFi();
void initTime();
bool hasValidSystemTime();
void retryTimeSyncIfNeeded();
void controlGate(bool open);
void checkGateAutoClose();
void updateDisplay();
void updateParkingSlots();
void checkRFID();
bool handleRFIDAccess(String cardUID);

// Firestore functions
String getISOTimestamp();
String firestoreGet(String path);
bool firestorePatch(String path, String jsonPayload);
bool firestorePost(String collectionPath, String jsonPayload);
void syncSlotsFromFirestore();
void updateSlotInFirestore(int slotIndex);
void sendHeartbeat();
void sendPendingRFID(String rfidUID);
void logActivity(String action, int slotIndex, String details);
String lookupUserByRFID(String rfidTag);
void checkGateCommands();  // NEW: Listen for remote gate control
void checkBookingTimeouts();  // NEW: Auto-cancel expired bookings
void forceSyncAllSlotsToFirestore();  // NEW: Admin one-click slot state repair
time_t parseISO8601ToEpoch(const String& isoTime);

// Utility functions
void saveToPreferences();
void loadFromPreferences();
String getDurationString(unsigned long startMs, unsigned long endMs);

// ==================== SETUP ====================
void setup() {
  Serial.begin(115200);
  
  Serial.println("\n\n=========================================");
  Serial.println("  ESP32 SMART PARKING v5.0 - FIRESTORE  ");
  Serial.println("=========================================\n");
  
  preferences.begin("parking", false);
  randomSeed(micros());
  initHardware();
  connectToWiFi();
  initTime();
  loadFromPreferences();
  
  // Initial Firestore sync
  sendHeartbeat();
  syncSlotsFromFirestore();
  // Stagger next command poll to avoid immediate read collisions after boot.
  lastGateCommandCheck = millis() + (unsigned long)random(3000, 9000);
  
  Serial.println("\n✅ System Ready!");
  Serial.println("📡 Using Firestore REST API");
  Serial.println("🌐 IP: " + WiFi.localIP().toString());
  
  lcd.clear();
  lcd.print("System Ready!");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP());
  delay(2000);
}

void loop() {
  unsigned long currentMillis = millis();
  bool inFirestoreBackoff = (firestoreBackoffUntil > currentMillis);

  // Keep trying NTP in the background if boot-time sync failed.
  retryTimeSyncIfNeeded();
  
  // Check RFID (continuous)
  checkRFID();
  
  // Update parking slots from IR sensors (continuous)
  updateParkingSlots();
  
  // Auto-close gate after timeout
  checkGateAutoClose();
  
  // Update LCD display
  if (currentMillis - lastDisplayUpdate >= DISPLAY_INTERVAL) {
    updateDisplay();
    lastDisplayUpdate = currentMillis;
  }
  
  // Send heartbeat to Firestore even during GET backoff.
  // This keeps dashboard online/offline state accurate while reads are throttled.
  if (currentMillis - lastHeartbeat >= HEARTBEAT_INTERVAL) {
    sendHeartbeat();
    lastHeartbeat = currentMillis;
  }
  
  // Sync slot bookings from Firestore (web app bookings)
  if (!inFirestoreBackoff &&
      (currentMillis - lastFirestoreGetAt >= FIRESTORE_GET_MIN_GAP) &&
      currentMillis - lastSlotSync >= SLOT_SYNC_INTERVAL) {
    syncSlotsFromFirestore();
    lastSlotSync = currentMillis;
  }
  
  // Check for remote gate control commands from admin panel
  if (!inFirestoreBackoff &&
      (currentMillis - lastFirestoreGetAt >= FIRESTORE_GET_MIN_GAP) &&
      currentMillis - lastGateCommandCheck >= GATE_COMMAND_INTERVAL) {
    checkGateCommands();
    lastGateCommandCheck = currentMillis;
  }
  
  // Check for expired bookings (every minute)
  if (currentMillis - lastBookingCheck >= BOOKING_CHECK_INTERVAL) {
    checkBookingTimeouts();
    lastBookingCheck = currentMillis;
  }
  
  delay(50);
}

// ==================== HARDWARE INIT ====================
void initHardware() {
  lcd.begin(16, 2);
  lcd.clear();
  lcd.print("Smart Parking");
  lcd.setCursor(0, 1);
  lcd.print("v5.0 Firestore");
  
  // Initialize IR sensors and LEDs
  for (int i = 0; i < 4; i++) {
    pinMode(slots[i].irPin, INPUT_PULLUP);
    pinMode(slots[i].ledPin, OUTPUT);
    digitalWrite(slots[i].ledPin, LOW);
    
    // Read initial IR state
    delay(10); // Small delay for stable reading
    bool currentIR = digitalRead(slots[i].irPin);
    slots[i].lastIRState = currentIR;
    slots[i].lastIRChange = millis();
    
    // Initialize slot availability based on actual IR sensor state
    // LOW = vehicle present = not available
    // HIGH = no vehicle = available
    slots[i].isAvailable = (currentIR == HIGH);
    
    // Set LED based on initial state
    digitalWrite(slots[i].ledPin, currentIR == LOW ? HIGH : LOW);
    
    Serial.printf("🚗 Slot %s INIT: IR_PIN=%d, IR_RAW=%d, IR=%s, Available=%s, LED=%s\n", 
                  slots[i].slotName.c_str(),
                  slots[i].irPin,
                  currentIR,
                  currentIR == LOW ? "BLOCKED" : "CLEAR",
                  slots[i].isAvailable ? "YES" : "NO",
                  currentIR == LOW ? "ON" : "OFF");
  }
  
  // Initialize SPI for RFID with explicit pins
  // MFRC522 uses hardware SPI: SCK=18, MISO=19, MOSI=23, SS=5
  SPI.begin(18, 19, 23, 5);  // SCK, MISO, MOSI, SS
  mfrc522.PCD_Init();
  delay(100);
  
  // Check RFID reader - try multiple times
  byte version = 0;
  for (int i = 0; i < 3; i++) {
    version = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
    if (version == 0x91 || version == 0x92) break;
    delay(100);
    mfrc522.PCD_Init();
  }
  
  Serial.printf("MFRC522 Version: 0x%02X ", version);
  if (version == 0x91 || version == 0x92) {
    Serial.println("(OK)");
    rfidReaderOK = true;
  } else if (version == 0x00 || version == 0xFF || version == 0xEE) {
    Serial.println("(ERROR - Check SPI wiring: SDA=5, SCK=18, MOSI=23, MISO=19, RST=4)");
    rfidReaderOK = false;
  } else {
    Serial.printf("(Unknown - may still work)\n");
    rfidReaderOK = true;  // Try anyway
  }
  
  // Initialize servo
  gateServo.attach(SERVO_PIN);
  gateServo.write(90); // Closed position
  delay(500);
  
  Serial.println("✅ Hardware initialized");
}

void connectToWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("🌐 Connecting to WiFi: ");
  Serial.println(ssid);
  
  lcd.clear();
  lcd.print("Connecting WiFi");
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    lcd.setCursor(attempts % 16, 1);
    lcd.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi Connected!");
    Serial.println("   IP: " + WiFi.localIP().toString());
    
    lcd.clear();
    lcd.print("WiFi Connected!");
    lcd.setCursor(0, 1);
    lcd.print(WiFi.localIP());
    delay(2000);
  } else {
    Serial.println("\n❌ WiFi Failed!");
    lcd.clear();
    lcd.print("WiFi Failed!");
    lcd.setCursor(0, 1);
    lcd.print("Check settings");
  }
  
  // Skip SSL certificate validation (for development)
  wifiClient.setInsecure();
}

void initTime() {
  // Configure NTP for proper timestamps
  configTime(0, 0, "pool.ntp.org", "time.nist.gov", "time.google.com");
  lastTimeSyncAttempt = millis();
  Serial.print("⏰ Syncing time");
  
  int attempts = 0;
  while (!hasValidSystemTime() && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (hasValidSystemTime()) {
    Serial.println(" ✅");
  } else {
    Serial.println(" ⚠️ Time sync failed");
  }
}

bool hasValidSystemTime() {
  return time(nullptr) >= MIN_VALID_UNIX_TIME;
}

void retryTimeSyncIfNeeded() {
  if (hasValidSystemTime()) return;

  unsigned long nowMs = millis();
  if (nowMs - lastTimeSyncAttempt < TIME_SYNC_RETRY_INTERVAL) return;

  lastTimeSyncAttempt = nowMs;
  Serial.println("⏰ Retrying NTP sync...");
  configTime(0, 0, "pool.ntp.org", "time.nist.gov", "time.google.com");
}

// ==================== GATE CONTROL ====================
void controlGate(bool open) {
  if (open && !gateOpen) {
    gateServo.write(0); // Open position
    gateOpen = true;
    gateOpenTime = millis();
    Serial.println("🚪 Gate OPENED");
    
    lcd.clear();
    lcd.print("Gate Open");
    lcd.setCursor(0, 1);
    lcd.print("Welcome!");
  } else if (!open && gateOpen) {
    gateServo.write(90); // Closed position
    gateOpen = false;
    Serial.println("🚪 Gate CLOSED");
  }
}

void checkGateAutoClose() {
  if (gateOpen && (millis() - gateOpenTime >= GATE_OPEN_DURATION)) {
    Serial.println("⏰ Auto-closing gate");
    controlGate(false);
  }
}

// ==================== DISPLAY ====================
void updateDisplay() {
  lcd.clear();
  
  // Count available slots
  availableSlots = 0;
  for (int i = 0; i < 4; i++) {
    if (slots[i].isAvailable) availableSlots++;
  }
  
  switch (displayMode) {
    case 0:
      lcd.print("Parking: ");
      lcd.print(availableSlots);
      lcd.print("/4");
      lcd.setCursor(0, 1);
      lcd.print("A:");
      lcd.print(slots[0].isAvailable ? "O" : (slots[0].currentBooking.userId.length() > 0 ? "B" : "X"));
      lcd.print(" B:");
      lcd.print(slots[1].isAvailable ? "O" : (slots[1].currentBooking.userId.length() > 0 ? "B" : "X"));
      lcd.print(" C:");
      lcd.print(slots[2].isAvailable ? "O" : (slots[2].currentBooking.userId.length() > 0 ? "B" : "X"));
      lcd.print(" D:");
      lcd.print(slots[3].isAvailable ? "O" : (slots[3].currentBooking.userId.length() > 0 ? "B" : "X"));
      break;
      
    case 1:
      lcd.print("Gate: ");
      lcd.print(gateOpen ? "OPEN" : "CLOSED");
      lcd.setCursor(0, 1);
      lcd.print("User: ");
      lcd.print(lastUser.substring(0, 10));
      break;
      
    case 2:
      lcd.print("Entries: ");
      lcd.print(totalEntries);
      lcd.setCursor(0, 1);
      lcd.print("Exits: ");
      lcd.print(totalExits);
      break;
  }
  
  displayMode = (displayMode + 1) % 3;
}

// ==================== PARKING SLOTS (IR SENSORS) ====================
void updateParkingSlots() {
  static unsigned long lastCheck = 0;
  static unsigned long lastDebugPrint = 0;
  
  if (millis() - lastCheck < 100) return;
  lastCheck = millis();
  
  // Debug print every 5 seconds
  bool printDebug = (millis() - lastDebugPrint >= 5000);
  if (printDebug) {
    lastDebugPrint = millis();
    Serial.print("IR Status: ");
  }
  
  for (int i = 0; i < 4; i++) {
    bool currentIRState = digitalRead(slots[i].irPin);
    bool vehiclePresent = (currentIRState == LOW); // LOW = vehicle detected
    bool hasBooking = slots[i].currentBooking.userId.length() > 0;
    
    if (printDebug) {
      Serial.printf("%s=%s ", slots[i].slotName.c_str(), vehiclePresent ? "BLOCKED" : "CLEAR");
    }
    
    // LED ON when:
    // 1. Slot has an active booking (user booked via web app)
    // 2. OR vehicle is physically present (IR sensor blocked)
    bool ledShouldBeOn = hasBooking || vehiclePresent;
    digitalWrite(slots[i].ledPin, ledShouldBeOn ? HIGH : LOW);
    
    // Detect state change
    if (currentIRState != slots[i].lastIRState) {
      slots[i].lastIRChange = millis();
      slots[i].lastIRState = currentIRState;
      Serial.printf("\n⚡ Slot %s IR changed to %s\n", 
                    slots[i].slotName.c_str(), 
                    vehiclePresent ? "BLOCKED" : "CLEAR");
    }
    
    // Debounce: wait 1 second before confirming state change
    if (millis() - slots[i].lastIRChange > 1000) {
      // Physical entry detection is based on localEntryTime, not isAvailable.
      // This prevents clearing a web booking when the slot is reserved but no car has entered yet.
      if (vehiclePresent && slots[i].localEntryTime == 0) {
        slots[i].isAvailable = false;
        slots[i].localEntryTime = millis();
        
        Serial.println("✅ Vehicle ENTERED Slot " + slots[i].slotName);
        
        // Log entry with booking info if available
        String details = "Vehicle entered";
        if (slots[i].currentBooking.userName.length() > 0) {
          details = "User: " + slots[i].currentBooking.userName;
          lastUser = slots[i].currentBooking.userName;
        }
        
        logActivity("entry_detected", i, details);
        totalEntries++;
        updateSlotInFirestore(i);
        saveToPreferences();
      }
      // Physical exit only if a physical entry had been detected previously.
      else if (!vehiclePresent && slots[i].localEntryTime > 0) {
        Serial.println("🚗 Vehicle EXITED Slot " + slots[i].slotName);
        
        // Calculate duration
        String duration = getDurationString(slots[i].localEntryTime, millis());
        String details = "Duration: " + duration;
        if (slots[i].currentBooking.userName.length() > 0) {
          details = "User: " + slots[i].currentBooking.userName + ", " + details;
        }
        
        logActivity("exit_completed", i, details);
        totalExits++;
        
        // Clear slot data
        slots[i].localEntryTime = 0;
        slots[i].currentBooking = {"", "", "", "", "", ""};
        // LED is now controlled at the top of the loop based on IR state
        
        updateSlotInFirestore(i);
        saveToPreferences();
      }

      // Keep availability consistent with both booking state and physical presence.
      hasBooking = slots[i].currentBooking.userId.length() > 0;
      slots[i].isAvailable = (!vehiclePresent && !hasBooking);
    }
  }
  
  if (printDebug) {
    Serial.println();
  }
}

// ==================== RFID ====================
void checkRFID() {
  // Clear old RFID after cooldown
  if (rfidDetected && (millis() - rfidDetectedTime > RFID_COOLDOWN)) {
    rfidDetected = false;
    currentRFID = "";
  }
  
  if (!mfrc522.PICC_IsNewCardPresent()) return;
  if (!mfrc522.PICC_ReadCardSerial()) return;
  
  // Build RFID UID string
  String cardUID = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    cardUID += (mfrc522.uid.uidByte[i] < 0x10 ? "0" : "") + String(mfrc522.uid.uidByte[i], HEX);
  }
  cardUID.toUpperCase();
  
  // Prevent duplicate scans
  if (cardUID != currentRFID) {
    currentRFID = cardUID;
    rfidDetected = true;
    rfidDetectedTime = millis();
    
    Serial.println("\n🎫 RFID DETECTED: " + cardUID);
    
    // Log current booking states for debugging
    Serial.println("📋 Current bookings:");
    for (int i = 0; i < 4; i++) {
      if (slots[i].currentBooking.userId.length() > 0) {
        Serial.printf("   Slot %s: User=%s, RFID=%s, Available=%s\n",
                      slots[i].slotName.c_str(),
                      slots[i].currentBooking.userName.c_str(),
                      slots[i].currentBooking.rfidTag.c_str(),
                      slots[i].isAvailable ? "YES" : "NO");
      }
    }
    
    // First check if this RFID has an active booking (registered user)
    bool hasBooking = handleRFIDAccess(cardUID);
    
    // Only send to pendingRFID if card is NOT already registered/has no booking
    // This prevents overwriting pendingRFID when an existing user scans
    if (!hasBooking) {
      sendPendingRFID(cardUID);
    }
  }
  
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}

// Returns true if RFID has a booking or is a registered user, false if unknown
bool handleRFIDAccess(String cardUID) {
  // First check local cache for booking
  for (int i = 0; i < 4; i++) {
    // Check if RFID matches a booked slot - look for valid booking with matching RFID
    // Don't rely on isAvailable flag, check if there's an actual booking
    bool hasValidBooking = (slots[i].currentBooking.userId.length() > 0);
    bool rfidMatches = (slots[i].currentBooking.rfidTag == cardUID);
    
    if (hasValidBooking && rfidMatches) {
      String userName = slots[i].currentBooking.userName;
      
      Serial.printf("✅ ACCESS GRANTED (cached): %s for Slot %s\n", 
                    userName.c_str(), slots[i].slotName.c_str());
      lastUser = userName;
      
      lcd.clear();
      lcd.print("Welcome!");
      lcd.setCursor(0, 1);
      lcd.print(userName.substring(0, 16));
      
      // Update entry time in Firestore
      slots[i].currentBooking.entryTime = getISOTimestamp();
      logActivity("entry_granted", i, "RFID access granted");
      
      // CRITICAL FIX: Open the gate!
      controlGate(true);
      
      // Update slot in Firestore to record entry time
      updateSlotInFirestore(i);
      
      return true;  // Has booking
    }
  }
  
  // Look up user in Firestore
  String userInfo = lookupUserByRFID(cardUID);
  
  if (userInfo.length() > 0) {
    // Parse user info and check for active booking
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, userInfo);
    
    String userName = doc["userName"] | "User";
    String userId = doc["userId"] | "";
    
    // Check if user has any booked slot
    for (int i = 0; i < 4; i++) {
      if (slots[i].currentBooking.userId == userId) {
        Serial.println("✅ ACCESS GRANTED: " + userName);
        lastUser = userName;
        
        lcd.clear();
        lcd.print("Welcome!");
        lcd.setCursor(0, 1);
        lcd.print(userName.substring(0, 16));
        
        slots[i].currentBooking.entryTime = getISOTimestamp();
        logActivity("entry_granted", i, "RFID: " + cardUID);
        updateSlotInFirestore(i);
        controlGate(true);
        return true;  // Has booking
      }
    }
    
    // User found but no booking - still a registered user
    lcd.clear();
    lcd.print("No Booking");
    lcd.setCursor(0, 1);
    lcd.print("Book slot first");
    Serial.println("⚠️ User found but no active booking");
    return true;  // Registered user (don't send to pendingRFID)
  } else {
    // Unknown RFID - show on LCD for registration
    lcd.clear();
    lcd.print("RFID Detected!");
    lcd.setCursor(0, 1);
    lcd.print(cardUID.substring(0, 16));
    Serial.println("📋 Unknown RFID - available for registration");
    return false;  // Unknown card - send to pendingRFID
  }
}

// ==================== FIRESTORE API FUNCTIONS ====================

String getISOTimestamp() {
  time_t now = time(nullptr);
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  
  char timestamp[30];
  strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%SZ", &timeinfo);
  return String(timestamp);
}

String firestoreGet(String path) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("❌ WiFi not connected");
    return "";
  }
  
  HTTPClient http;
  String url = FIRESTORE_BASE_URL + path + "?key=" + FIREBASE_API_KEY;
  
  http.begin(wifiClient, url);
  http.setTimeout(5000);
  
  int httpCode = http.GET();
  lastFirestoreGetAt = millis();
  lastFirestoreGetHttpCode = httpCode;
  String response = "";
  
  if (httpCode == HTTP_CODE_OK) {
    firestoreBackoffMs = 0;
    firestoreBackoffUntil = 0;
    response = http.getString();
  } else if (httpCode == HTTP_CODE_NOT_FOUND) {
    firestoreBackoffMs = 0;
    firestoreBackoffUntil = 0;
  } else if (httpCode > 0) {
    Serial.println("⚠️ GET " + path + " - HTTP " + String(httpCode));
    if (httpCode == 429) {
      if (firestoreBackoffMs == 0) firestoreBackoffMs = 2000;
      else firestoreBackoffMs = min(firestoreBackoffMs * 2, 60000UL);
      firestoreBackoffUntil = millis() + firestoreBackoffMs;
      Serial.println("⏳ Firestore backoff " + String(firestoreBackoffMs) + "ms");
    }
  } else {
    Serial.println("❌ GET failed: " + http.errorToString(httpCode));
  }
  
  http.end();
  return response;
}

bool firestorePatch(String path, String jsonPayload) {
  if (WiFi.status() != WL_CONNECTED) return false;
  
  HTTPClient http;
  String url = FIRESTORE_BASE_URL + path + "?key=" + FIREBASE_API_KEY;
  
  http.begin(wifiClient, url);
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(5000);
  
  int httpCode = http.PATCH(jsonPayload);
  bool success = (httpCode == HTTP_CODE_OK || httpCode == 200);
  
  if (!success && httpCode > 0) {
    Serial.println("⚠️ PATCH " + path + " - HTTP " + String(httpCode));
    if (httpCode == 429) {
      if (firestoreBackoffMs == 0) firestoreBackoffMs = 2000;
      else firestoreBackoffMs = min(firestoreBackoffMs * 2, 60000UL);
      firestoreBackoffUntil = millis() + firestoreBackoffMs;
    }
  } else if (success) {
    firestoreBackoffMs = 0;
    firestoreBackoffUntil = 0;
  }
  
  http.end();
  return success;
}

bool firestorePost(String collectionPath, String jsonPayload) {
  if (WiFi.status() != WL_CONNECTED) return false;
  
  HTTPClient http;
  String url = FIRESTORE_BASE_URL + collectionPath + "?key=" + FIREBASE_API_KEY;
  
  http.begin(wifiClient, url);
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(5000);
  
  int httpCode = http.POST(jsonPayload);
  bool success = (httpCode == HTTP_CODE_OK || httpCode == 200);
  
  if (!success && httpCode > 0) {
    Serial.println("⚠️ POST " + collectionPath + " - HTTP " + String(httpCode));
  }
  
  http.end();
  return success;
}

// ==================== FIRESTORE SYNC FUNCTIONS ====================

void syncSlotsFromFirestore() {
  if (WiFi.status() != WL_CONNECTED) return;
  
  static unsigned long lastDebug = 0;
  bool showDebug = (millis() - lastDebug > 10000); // Debug every 10 seconds
  if (showDebug) {
    Serial.println("🔄 Syncing slots from Firestore...");
    lastDebug = millis();
  }
  
  // Use one collection read to avoid 4x per-cycle document GET burst (helps prevent HTTP 429).
  String response = firestoreGet("/parkingSlots");
  if (response.length() == 0) return;

  DynamicJsonDocument doc(16384);
  DeserializationError error = deserializeJson(doc, response);
  if (error) {
    Serial.println("⚠️ Failed to parse /parkingSlots response");
    return;
  }

  if (!doc.containsKey("documents")) {
    return;
  }

  JsonArray docs = doc["documents"].as<JsonArray>();
  bool updatedAny = false;

  for (JsonObject slotDoc : docs) {
    String fullName = slotDoc["name"] | "";
    int slashPos = fullName.lastIndexOf('/');
    if (slashPos < 0) continue;
    String docId = fullName.substring(slashPos + 1);

    int slotIndex = -1;
    if (docId == "slot-a") slotIndex = 0;
    else if (docId == "slot-b") slotIndex = 1;
    else if (docId == "slot-c") slotIndex = 2;
    else if (docId == "slot-d") slotIndex = 3;
    else continue;

    JsonObject fields = slotDoc["fields"];
    if (!fields) continue;

    JsonObject booking;
    if (fields.containsKey("currentBooking")) {
      booking = fields["currentBooking"]["mapValue"]["fields"];
    }
    bool hasRemoteVehiclePresent = false;
    bool remoteVehiclePresent = false;
    if (fields.containsKey("vehiclePresent") && fields["vehiclePresent"].containsKey("booleanValue")) {
      hasRemoteVehiclePresent = true;
      remoteVehiclePresent = fields["vehiclePresent"]["booleanValue"] | false;
    }
    bool hasRemoteIsAvailable = false;
    bool remoteIsAvailable = true;
    if (fields.containsKey("isAvailable") && fields["isAvailable"].containsKey("booleanValue")) {
      hasRemoteIsAvailable = true;
      remoteIsAvailable = fields["isAvailable"]["booleanValue"] | true;
    }
    bool hasRemoteStatus = false;
    String remoteStatus = "";
    if (fields.containsKey("status") && fields["status"].containsKey("stringValue")) {
      hasRemoteStatus = true;
      remoteStatus = fields["status"]["stringValue"].as<String>();
    }
    String newUserId = "";
    String newUserName = "";
    String newVehicle = "";
    String newRfid = "";
    String newBookedAt = "";

    if (!booking.isNull() && booking.containsKey("userId") && booking["userId"].containsKey("stringValue")) {
      newUserId = booking["userId"]["stringValue"].as<String>();
    }
    if (!booking.isNull() && booking.containsKey("userName") && booking["userName"].containsKey("stringValue")) {
      newUserName = booking["userName"]["stringValue"].as<String>();
    }
    if (!booking.isNull() && booking.containsKey("vehicleNumber") && booking["vehicleNumber"].containsKey("stringValue")) {
      newVehicle = booking["vehicleNumber"]["stringValue"].as<String>();
    }
    if (!booking.isNull() && booking.containsKey("rfidTag") && booking["rfidTag"].containsKey("stringValue")) {
      newRfid = booking["rfidTag"]["stringValue"].as<String>();
    }
    if (!booking.isNull() && booking.containsKey("bookedAt") && booking["bookedAt"].containsKey("timestampValue")) {
      newBookedAt = booking["bookedAt"]["timestampValue"].as<String>();
    } else if (!booking.isNull() && booking.containsKey("bookedAt") && booking["bookedAt"].containsKey("stringValue")) {
      newBookedAt = booking["bookedAt"]["stringValue"].as<String>();
    }

    bool wasAvailable = slots[slotIndex].currentBooking.userId.length() == 0;
    bool nowBooked = newUserId.length() > 0;

    if (nowBooked) {
      Serial.printf("🔍 Slot %s booking: userId=%s, userName=%s, rfid=%s\n",
                    slots[slotIndex].slotName.c_str(), newUserId.c_str(), newUserName.c_str(), newRfid.c_str());
    }

    if (wasAvailable && nowBooked) {
      Serial.println("📅 Web booking detected: Slot " + slots[slotIndex].slotName + " by " + newUserName);
      digitalWrite(slots[slotIndex].ledPin, HIGH);
      logActivity("slot_booked", slotIndex, "Booked via web by " + newUserName);
    }

    bool wasBooked = slots[slotIndex].currentBooking.userId.length() > 0;
    if (wasBooked && !nowBooked) {
      Serial.println("🔓 Booking cleared: Slot " + slots[slotIndex].slotName);
    }

    slots[slotIndex].currentBooking.userId = newUserId;
    slots[slotIndex].currentBooking.userName = newUserName;
    slots[slotIndex].currentBooking.vehicleNumber = newVehicle;
    slots[slotIndex].currentBooking.rfidTag = newRfid;
    slots[slotIndex].currentBooking.bookedAt = newBookedAt;

    // Read current physical state
    bool vehiclePresent = (digitalRead(slots[slotIndex].irPin) == LOW);
    
    // Slot is NOT available if:
    // 1. It has an active booking (nowBooked), OR
    // 2. A vehicle is physically present
    // This ensures reserved slots show as unavailable even before vehicle arrives
    bool localIsAvailable = (!nowBooked && !vehiclePresent);
    slots[slotIndex].isAvailable = localIsAvailable;

    String localStatus = vehiclePresent ? "occupied" : (nowBooked ? "reserved" : "available");

    bool firestoreMismatch =
      (!hasRemoteVehiclePresent || remoteVehiclePresent != vehiclePresent) ||
      (!hasRemoteIsAvailable || remoteIsAvailable != localIsAvailable) ||
      (!hasRemoteStatus || remoteStatus != localStatus);

    if (firestoreMismatch) {
      Serial.printf("🛠️ Repairing Firestore slot %s (remote: status=%s, vehicle=%s, available=%s | local: status=%s, vehicle=%s, available=%s)\n",
                    slots[slotIndex].slotName.c_str(),
                    remoteStatus.c_str(),
                    remoteVehiclePresent ? "YES" : "NO",
                    remoteIsAvailable ? "YES" : "NO",
                    localStatus.c_str(),
                    vehiclePresent ? "YES" : "NO",
                    localIsAvailable ? "YES" : "NO");
      updateSlotInFirestore(slotIndex);
    }
    
    Serial.printf("🔄 Slot %s sync: booked=%s, vehicle=%s, available=%s\n",
                  slots[slotIndex].slotName.c_str(),
                  nowBooked ? "YES" : "NO",
                  vehiclePresent ? "YES" : "NO",
                  slots[slotIndex].isAvailable ? "YES" : "NO");
    
    updatedAny = true;
  }

  if (updatedAny && showDebug) {
    Serial.println("✅ Slot sync applied from collection snapshot");
  }
}

void updateSlotInFirestore(int slotIndex) {
  if (WiFi.status() != WL_CONNECTED) return;
  
  // Build Firestore document JSON
  DynamicJsonDocument doc(1024);
  
  bool hasBooking = slots[slotIndex].currentBooking.userId.length() > 0;
  bool vehiclePresent = (digitalRead(slots[slotIndex].irPin) == LOW);
  bool isAvailableNow = slots[slotIndex].isAvailable;
  String statusValue = "available";
  if (vehiclePresent) statusValue = "occupied";
  else if (hasBooking) statusValue = "reserved";

  doc["fields"]["id"]["stringValue"] = slots[slotIndex].slotId;
  doc["fields"]["name"]["stringValue"] = slots[slotIndex].slotName;
  doc["fields"]["status"]["stringValue"] = statusValue;
  doc["fields"]["isAvailable"]["booleanValue"] = slots[slotIndex].isAvailable;
  doc["fields"]["vehiclePresent"]["booleanValue"] = vehiclePresent;
  doc["fields"]["lastUpdated"]["timestampValue"] = getISOTimestamp();
  
  if (hasBooking) {
    doc["fields"]["userId"]["stringValue"] = slots[slotIndex].currentBooking.userId;
    doc["fields"]["userName"]["stringValue"] = slots[slotIndex].currentBooking.userName;
    doc["fields"]["vehicleNumber"]["stringValue"] = slots[slotIndex].currentBooking.vehicleNumber;
    doc["fields"]["rfidTag"]["stringValue"] = slots[slotIndex].currentBooking.rfidTag;
  } else {
    doc["fields"]["userId"]["nullValue"] = nullptr;
    doc["fields"]["userName"]["nullValue"] = nullptr;
    doc["fields"]["vehicleNumber"]["nullValue"] = nullptr;
    doc["fields"]["rfidTag"]["nullValue"] = nullptr;
  }
  
  // Build currentBooking map
  JsonObject booking = doc["fields"]["currentBooking"]["mapValue"]["fields"].to<JsonObject>();
  
  if (hasBooking) {
    booking["userId"]["stringValue"] = slots[slotIndex].currentBooking.userId;
    booking["userName"]["stringValue"] = slots[slotIndex].currentBooking.userName;
    booking["vehicleNumber"]["stringValue"] = slots[slotIndex].currentBooking.vehicleNumber;
    booking["rfidTag"]["stringValue"] = slots[slotIndex].currentBooking.rfidTag;
    
    if (slots[slotIndex].currentBooking.bookedAt.length() > 0) {
      booking["bookedAt"]["timestampValue"] = slots[slotIndex].currentBooking.bookedAt;
    } else {
      booking["bookedAt"]["nullValue"] = nullptr;
    }
    
    if (slots[slotIndex].currentBooking.entryTime.length() > 0) {
      booking["entryTime"]["timestampValue"] = slots[slotIndex].currentBooking.entryTime;
    } else {
      booking["entryTime"]["nullValue"] = nullptr;
    }
  } else {
    // Clear booking
    booking["userId"]["nullValue"] = nullptr;
    booking["userName"]["nullValue"] = nullptr;
    booking["vehicleNumber"]["nullValue"] = nullptr;
    booking["rfidTag"]["nullValue"] = nullptr;
    booking["bookedAt"]["nullValue"] = nullptr;
    booking["entryTime"]["nullValue"] = nullptr;
  }
  
  String json;
  serializeJson(doc, json);
  
  bool ok = firestorePatch("/parkingSlots/" + slots[slotIndex].slotId, json);
  if (ok) {
    Serial.println("📤 Slot " + slots[slotIndex].slotName + " updated in Firestore");
  } else {
    Serial.println("⚠️ Slot " + slots[slotIndex].slotName + " update failed");
  }
}

void sendHeartbeat() {
  if (WiFi.status() != WL_CONNECTED) return;
  
  // Count available slots
  int available = 0;
  for (int i = 0; i < 4; i++) {
    if (slots[i].isAvailable) available++;
  }
  
  DynamicJsonDocument doc(512);
  bool timeSynced = hasValidSystemTime();
  doc["fields"]["isOnline"]["booleanValue"] = true;
  doc["fields"]["lastHeartbeat"]["timestampValue"] = getISOTimestamp();
  doc["fields"]["timeSynced"]["booleanValue"] = timeSynced;
  doc["fields"]["heartbeatUptimeMs"]["integerValue"] = String(millis());
  doc["fields"]["ipAddress"]["stringValue"] = WiFi.localIP().toString();
  doc["fields"]["deviceId"]["stringValue"] = DEVICE_ID;
  doc["fields"]["availableSlots"]["integerValue"] = String(available);
  doc["fields"]["gateOpen"]["booleanValue"] = gateOpen;
  
  String json;
  serializeJson(doc, json);
  
  bool ok = firestorePatch("/systemConfig/esp32Status", json);
  if (ok) {
    Serial.println("💓 Heartbeat sent");
    if (!timeSynced) {
      Serial.println("⚠️ Heartbeat timestamp may be inaccurate until NTP sync succeeds");
    }
  } else {
    Serial.println("⚠️ Heartbeat failed");
  }
}

void sendPendingRFID(String rfidUID) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("⚠️ Cannot send RFID - WiFi not connected");
    return;
  }
  
  DynamicJsonDocument doc(256);
  doc["fields"]["tag"]["stringValue"] = rfidUID;
  doc["fields"]["timestamp"]["timestampValue"] = getISOTimestamp();
  doc["fields"]["processed"]["booleanValue"] = false;
  doc["fields"]["deviceId"]["stringValue"] = DEVICE_ID;
  
  String json;
  serializeJson(doc, json);
  
  // Try PATCH first (updates existing doc)
  bool success = firestorePatch("/pendingRFID/latest", json);
  
  if (!success) {
    // If PATCH fails (doc doesn't exist), try creating via direct URL
    Serial.println("⚠️ PATCH failed, creating pendingRFID document...");
    
    HTTPClient http;
    // Use document creation endpoint with documentId parameter
    String url = String(FIRESTORE_BASE_URL) + "/pendingRFID?documentId=latest&key=" + FIREBASE_API_KEY;
    
    http.begin(wifiClient, url);
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(5000);
    
    int httpCode = http.POST(json);
    success = (httpCode == HTTP_CODE_OK || httpCode == 200);
    
    if (success) {
      Serial.println("✅ Created pendingRFID document");
    } else {
      Serial.printf("❌ Failed to create pendingRFID: HTTP %d\n", httpCode);
    }
    
    http.end();
  }
  
  if (success) {
    Serial.println("📤 RFID sent to pendingRFID: " + rfidUID);
  }
}

void logActivity(String action, int slotIndex, String details) {
  if (WiFi.status() != WL_CONNECTED) return;
  
  DynamicJsonDocument doc(1024);
  doc["fields"]["timestamp"]["timestampValue"] = getISOTimestamp();
  doc["fields"]["action"]["stringValue"] = action;
  doc["fields"]["slotId"]["stringValue"] = slots[slotIndex].slotId;
  doc["fields"]["details"]["stringValue"] = details;
  doc["fields"]["source"]["stringValue"] = "esp32";
  
  if (slots[slotIndex].currentBooking.userId.length() > 0) {
    doc["fields"]["userId"]["stringValue"] = slots[slotIndex].currentBooking.userId;
  } else {
    doc["fields"]["userId"]["nullValue"] = nullptr;
  }
  
  if (slots[slotIndex].currentBooking.rfidTag.length() > 0) {
    doc["fields"]["rfidTag"]["stringValue"] = slots[slotIndex].currentBooking.rfidTag;
  } else {
    doc["fields"]["rfidTag"]["stringValue"] = currentRFID;
  }
  
  String json;
  serializeJson(doc, json);
  
  firestorePost("/activityLogs", json);
  Serial.println("📝 Activity: " + action + " - Slot " + slots[slotIndex].slotName);
}

String lookupUserByRFID(String rfidTag) {
  // Query users collection for matching RFID
  String response = firestoreGet("/users");
  
  if (response.length() == 0) return "";
  
  DynamicJsonDocument doc(8192);
  DeserializationError error = deserializeJson(doc, response);
  
  if (error) return "";
  
  // Check if we got a list of documents
  if (doc.containsKey("documents")) {
    JsonArray users = doc["documents"];
    
    for (JsonObject user : users) {
      JsonObject fields = user["fields"];
      if (fields.containsKey("rfidTag")) {
        String userRFID = fields["rfidTag"]["stringValue"] | "";
        if (userRFID.equalsIgnoreCase(rfidTag)) {
          // Found matching user
          DynamicJsonDocument result(512);
          result["userId"] = fields["uid"]["stringValue"] | "";
          result["userName"] = fields["displayName"]["stringValue"] | "User";
          result["email"] = fields["email"]["stringValue"] | "";
          
          String output;
          serializeJson(result, output);
          return output;
        }
      }
    }
  }
  
  return "";
}

// ==================== GATE CONTROL LISTENER ====================
void checkGateCommands() {
  if (WiFi.status() != WL_CONNECTED) return;
  
  // Check for pending gate command in systemConfig/gateCommand
  String response = firestoreGet("/systemConfig/gateCommand");
  
  // If document doesn't exist (404), create it
  if (response.length() == 0 && lastFirestoreGetHttpCode == HTTP_CODE_NOT_FOUND) {
    Serial.println("📝 Creating gateCommand document...");
    DynamicJsonDocument initDoc(256);
    initDoc["fields"]["action"]["stringValue"] = "none";
    initDoc["fields"]["executed"]["booleanValue"] = true;
    initDoc["fields"]["createdAt"]["timestampValue"] = getISOTimestamp();
    initDoc["fields"]["createdBy"]["stringValue"] = DEVICE_ID;
    
    String json;
    serializeJson(initDoc, json);
    
    // Create the document with PUT (overwrites if exists)
    HTTPClient http;
    String url = FIRESTORE_BASE_URL + "/systemConfig/gateCommand?key=" + FIREBASE_API_KEY;
    http.begin(wifiClient, url);
    http.addHeader("Content-Type", "application/json");
    
    int httpCode = http.PATCH(json);
    if (httpCode > 0 && httpCode < 300) {
      Serial.println("✅ gateCommand document created");
    } else {
      Serial.println("❌ Failed to create gateCommand: " + String(httpCode));
    }
    http.end();
    return;
  }
  if (response.length() == 0) return;
  
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, response);
  
  if (error) return;
  
  JsonObject fields = doc["fields"];
  if (!fields) return;
  
  // Check if there's an unexecuted command
  bool executed = true;
  if (fields.containsKey("executed")) {
    executed = fields["executed"]["booleanValue"] | true;
  }
  
  if (executed) return; // Already executed, skip
  
  // Get the action
  String action = "";
  if (fields.containsKey("action")) {
    action = fields["action"]["stringValue"].as<String>();
  }
  
  if (action.length() == 0) return;
  
  // Execute the gate command
  if (action == "OPEN" || action == "open") {
    Serial.println("🚪 Remote command: OPEN GATE");
    controlGate(true);
    
    lcd.clear();
    lcd.print("Remote Open");
    lcd.setCursor(0, 1);
    lcd.print("Admin Control");
  } 
  else if (action == "CLOSE" || action == "close") {
    Serial.println("🚪 Remote command: CLOSE GATE");
    controlGate(false);
    
    lcd.clear();
    lcd.print("Remote Close");
    lcd.setCursor(0, 1);
    lcd.print("Admin Control");
  }
  else if (action == "SYNC_SLOTS" || action == "sync_slots" || action == "SYNC" || action == "sync") {
    Serial.println("🛠️ Remote command: SYNC_SLOTS");
    forceSyncAllSlotsToFirestore();

    lcd.clear();
    lcd.print("Slots Synced");
    lcd.setCursor(0, 1);
    lcd.print("From Hardware");
  }
  else {
    Serial.println("⚠️ Unknown gateCommand action: " + action);
  }
  
  // Mark command as executed
  DynamicJsonDocument updateDoc(256);
  updateDoc["fields"]["executed"]["booleanValue"] = true;
  updateDoc["fields"]["executedAt"]["timestampValue"] = getISOTimestamp();
  updateDoc["fields"]["executedBy"]["stringValue"] = DEVICE_ID;
  
  String json;
  serializeJson(updateDoc, json);
  
  firestorePatch("/systemConfig/gateCommand", json);
  Serial.println("✅ Gate command executed and marked complete");
}

void forceSyncAllSlotsToFirestore() {
  for (int i = 0; i < 4; i++) {
    bool vehiclePresent = (digitalRead(slots[i].irPin) == LOW);
    bool hasBooking = slots[i].currentBooking.userId.length() > 0;
    slots[i].isAvailable = (!vehiclePresent && !hasBooking);

    // Keep LED consistent immediately with current physical/booking state.
    digitalWrite(slots[i].ledPin, (hasBooking || vehiclePresent) ? HIGH : LOW);

    updateSlotInFirestore(i);
  }

  Serial.println("✅ Forced slot state sync completed for all slots");
}

// ==================== BOOKING TIMEOUT ====================
void checkBookingTimeouts() {
  const unsigned long BOOKING_TIMEOUT = 300000;  // 5 minutes in milliseconds
  time_t nowEpoch = time(nullptr);
  if (nowEpoch < 1000000000) return;
  
  for (int i = 0; i < 4; i++) {
    // Check if slot has a booking but vehicle hasn't arrived
    // (slot is marked unavailable due to booking, but IR shows no vehicle)
    bool hasBooking = slots[i].currentBooking.userId.length() > 0;
    bool vehiclePresent = (digitalRead(slots[i].irPin) == LOW);
    
    if (hasBooking && !vehiclePresent) {
      time_t bookedAtEpoch = parseISO8601ToEpoch(slots[i].currentBooking.bookedAt);
      if (bookedAtEpoch == 0) continue;

      long ageSeconds = (long)(nowEpoch - bookedAtEpoch);
      if (ageSeconds < 0) ageSeconds = 0;
      if ((unsigned long)ageSeconds * 1000UL < BOOKING_TIMEOUT) continue;

      Serial.println("⏰ Slot " + slots[i].slotName + " has booking but no vehicle");
      
      // Log the timeout and clear the booking
      String details = "Booking expired - User: " + slots[i].currentBooking.userName;
      logActivity("booking_timeout", i, details);
      
      // Clear the booking locally
      slots[i].isAvailable = true;
      slots[i].currentBooking = {"", "", "", "", "", ""};
      
      // Turn off LED
      digitalWrite(slots[i].ledPin, LOW);
      
      // Update Firestore to clear the booking
      updateSlotInFirestore(i);
      
      Serial.println("🗑️ Booking cleared for Slot " + slots[i].slotName);
      
      lcd.clear();
      lcd.print("Booking Expired");
      lcd.setCursor(0, 1);
      lcd.print("Slot " + slots[i].slotName + " Free");
    }
  }
}

time_t parseISO8601ToEpoch(const String& isoTime) {
  if (isoTime.length() < 19) return 0;
  
  int year, month, day, hour, minute, second;
  int parsed = sscanf(
    isoTime.c_str(),
    "%d-%d-%dT%d:%d:%d",
    &year, &month, &day, &hour, &minute, &second
  );
  if (parsed != 6) return 0;

  struct tm tmTime;
  memset(&tmTime, 0, sizeof(tmTime));
  tmTime.tm_year = year - 1900;
  tmTime.tm_mon = month - 1;
  tmTime.tm_mday = day;
  tmTime.tm_hour = hour;
  tmTime.tm_min = minute;
  tmTime.tm_sec = second;
  tmTime.tm_isdst = 0;

  return mktime(&tmTime);
}

// ==================== PERSISTENCE ====================
void saveToPreferences() {
  preferences.putInt("totalEntries", totalEntries);
  preferences.putInt("totalExits", totalExits);
  preferences.putString("lastUser", lastUser);
}

void loadFromPreferences() {
  totalEntries = preferences.getInt("totalEntries", 0);
  totalExits = preferences.getInt("totalExits", 0);
  lastUser = preferences.getString("lastUser", "None");
  
  Serial.println("📂 Loaded from storage:");
  Serial.println("   Entries: " + String(totalEntries));
  Serial.println("   Exits: " + String(totalExits));
}

// ==================== UTILITIES ====================
String getDurationString(unsigned long startMs, unsigned long endMs) {
  unsigned long duration = (endMs - startMs) / 1000;
  unsigned long hours = duration / 3600;
  unsigned long minutes = (duration % 3600) / 60;
  unsigned long seconds = duration % 60;
  
  if (hours > 0) {
    return String(hours) + "h " + String(minutes) + "m";
  } else if (minutes > 0) {
    return String(minutes) + "m " + String(seconds) + "s";
  } else {
    return String(seconds) + "s";
  }
}
