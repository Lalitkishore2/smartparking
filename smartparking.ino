/*
 * ESP32 SMART PARKING SYSTEM v3.5 - WITH ADMIN SLOTS INTERFACE
 * - Servo opens instantly to 0° when RFID scanned
 * - Servo closes instantly to 90° after 5 seconds
 * - Admin dashboard requires password "0000"
 * - All other features: Profile edit, History tracking, etc.
 */

#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <LiquidCrystal.h>
#include <ESP32Servo.h>
#include <SPI.h>
#include <MFRC522.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>

// ==================== CONFIGURATION ====================
const char* ssid = "*********";
const char* password = "*******";
const char* hostname = "smartparking";

// Firebase Configuration
const char* DATABASE_URL = "https://******-rtdb.asia-southeast1.firebasedatabase.app/";

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
WebServer server(80);
Preferences preferences;

// ==================== DATA STRUCTURES ====================
struct ParkingSlot {
  int irPin;
  int ledPin;
  bool occupied;
  bool booked;
  String bookedBy;
  String bookedByUID;
  String vehicleNumber;
  unsigned long bookingTime;
  unsigned long occupiedTime;
  String slotName;
  bool lastIRState;
  unsigned long lastIRChange;
  String currentUserEmail;
};

ParkingSlot slots[4] = {
  {IR_SLOT_A, LED_SLOT_A, false, false, "", "", "", 0, 0, "A", false, 0, ""},
  {IR_SLOT_B, LED_SLOT_B, false, false, "", "", "", 0, 0, "B", false, 0, ""},  
  {IR_SLOT_C, LED_SLOT_C, false, false, "", "", "", 0, 0, "C", false, 0, ""},
  {IR_SLOT_D, LED_SLOT_D, false, false, "", "", "", 0, 0, "D", false, 0, ""}
};

struct User {
  String uid;
  String name;
  String email;
  String phone;
  String vehicleNumber;
  String rfidUID;
  String password;
};

struct ActivityLog {
  String activityType;
  String userName;
  String userEmail;
  String slotName;
  String vehicleNumber;
  String rfidUID;
  unsigned long timestamp;
  String duration;
};

// ==================== SYSTEM VARIABLES ====================
int availableSlots = 4;
bool gateOpen = false;
String lastUser = "None";
String currentRFID = "";
bool rfidDetected = false;
unsigned long rfidDetectedTime = 0;
const unsigned long RFID_HOLD_TIME = 10000;
int totalBookings = 0;
unsigned long systemStartTime = 0;
int totalEntries = 0;
int totalExits = 0;

// INSTANT SERVO CONTROL VARIABLES
unsigned long gateOpenTime = 0;
const unsigned long GATE_OPEN_DURATION = 5000; // 5 seconds

unsigned long lastDisplayUpdate = 0;
unsigned long lastFirebaseUpdate = 0;
unsigned long lastBookingCheck = 0;

int displayMode = 0;
std::vector<User> users;
std::vector<ActivityLog> activityHistory;
const int MAX_HISTORY = 100;

// ==================== FUNCTION DECLARATIONS ====================
void initHardware();
void connectToWiFi();
void setupMDNS();
void controlGate(bool open);
void checkGateAutoClose();
void updateDisplay();
void updateParkingSlots();
void checkRFID();
void handleRFIDAccess(String cardUID);
void checkBookingTimeouts();
User* findUserByEmail(String email);
User* findUserByRFID(String rfidUID);
void updateSlotInFirebase(int slotIndex);
void updateFirebaseStatus();
void updateUserInFirebase(const User& user);
void saveToPreferences();
void loadFromPreferences();
void setupWebServer();
void logActivity(String type, String userName, String userEmail, String slotName, String vehicleNumber, String rfidUID, String duration = "");
void saveActivityToFirebase(const ActivityLog& activity);
String getFormattedTime(unsigned long timestamp);
String getDurationString(unsigned long startTime, unsigned long endTime);
void handleHomePage();
void handleSignupPage();
void handleLoginPage();
void handleUserDashboard();
void handleAdminDashboard();
void handleAPICheckRFID();
void handleAPIRegister();
void handleAPILogin();
void handleAPIBookSlot();
void handleAPICancelBooking();
void handleAPISlots();
void handleAPIStatus();
void handleAPIAdminUsers();
void handleAPIAdminBookings();
void handleAPIHistory();
void handleAPIUserHistory();
void handleAPIGetUserProfile();
void handleAPIUpdateProfile();
void handleGateOpen();
void handleGateClose();

// ==================== SETUP ====================
void setup() {
  Serial.begin(115200);
  systemStartTime = millis();
  
  Serial.println("\n\n🚀 Starting Smart Parking System v3.5 (ADMIN SLOTS)...");
  
  preferences.begin("parking", false);
  initHardware();
  connectToWiFi();
  setupMDNS();
  setupWebServer();
  server.begin();
  loadFromPreferences();
  
  Serial.println("✅ System Ready!");
  Serial.println("🌐 Access: http://smartparking.local");
  Serial.println("🌐 IP: " + WiFi.localIP().toString());
  Serial.println("🔐 Admin Password: 0000");
}

void loop() {
  server.handleClient();
  checkRFID();
  updateParkingSlots();
  checkGateAutoClose();
  
  if (millis() - lastDisplayUpdate > 3000) {
    updateDisplay();
    lastDisplayUpdate = millis();
  }
  
  if (millis() - lastBookingCheck > 60000) {
    checkBookingTimeouts();
    lastBookingCheck = millis();
  }
  
  if (millis() - lastFirebaseUpdate > 10000) {
    updateFirebaseStatus();
    lastFirebaseUpdate = millis();
  }
  
  delay(50);
}

// ==================== HARDWARE INIT ====================
void initHardware() {
  lcd.begin(16, 2);
  lcd.clear();
  lcd.print("Smart Parking");
  lcd.setCursor(0, 1);
  lcd.print("Initializing...");
  
  for (int i = 0; i < 4; i++) {
    pinMode(slots[i].irPin, INPUT_PULLUP);
    pinMode(slots[i].ledPin, OUTPUT);
    digitalWrite(slots[i].ledPin, LOW);
    slots[i].lastIRState = digitalRead(slots[i].irPin);
    slots[i].lastIRChange = millis();
  }
  
  SPI.begin();
  mfrc522.PCD_Init();
  
  // INSTANT SERVO SETUP
  gateServo.attach(SERVO_PIN);
  gateServo.write(90); // Start closed
  
  Serial.println("✅ Hardware initialized");
}

void connectToWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("🌐 Connecting to WiFi");
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi Connected: " + WiFi.localIP().toString());
  } else {
    Serial.println("\n❌ WiFi Connection Failed");
  }
}

void setupMDNS() {
  if (MDNS.begin(hostname)) {
    MDNS.addService("http", "tcp", 80);
    Serial.println("✅ mDNS: smartparking.local");
  }
}

// ==================== INSTANT SERVO CONTROL ====================
void controlGate(bool open) {
  if (open && !gateOpen) {
    gateServo.write(0); // INSTANTLY open to 0°
    gateOpen = true;
    gateOpenTime = millis();
    Serial.println("🚪 Gate opened INSTANTLY");
  } else if (!open && gateOpen) {
    gateServo.write(90); // INSTANTLY close to 90°
    gateOpen = false;
    Serial.println("🚪 Gate closed INSTANTLY");
  }
}

void checkGateAutoClose() {
  if (gateOpen && (millis() - gateOpenTime >= GATE_OPEN_DURATION)) {
    Serial.println("⏰ Auto-closing gate after 5 seconds");
    controlGate(false);
  }
}

void updateParkingSlots() {
  static unsigned long lastCheck = 0;
  if (millis() - lastCheck < 100) return;
  lastCheck = millis();
  
  int newAvailableSlots = 0;
  
  for (int i = 0; i < 4; i++) {
    bool currentIRState = digitalRead(slots[i].irPin);
    
    if (currentIRState != slots[i].lastIRState) {
      slots[i].lastIRChange = millis();
      slots[i].lastIRState = currentIRState;
    }
    
    if (millis() - slots[i].lastIRChange > 1000) {
      bool isOccupied = (currentIRState == LOW);
      
      if (!slots[i].occupied && isOccupied) {
        slots[i].occupied = true;
        slots[i].occupiedTime = millis();
        digitalWrite(slots[i].ledPin, HIGH);
        
        Serial.println("✅ Vehicle ENTERED Slot " + slots[i].slotName);
        
        String userName = "Unknown";
        String userEmail = "";
        String vehicleNum = "";
        String rfidUID = "";
        
        if (slots[i].booked) {
          userName = slots[i].bookedBy;
          vehicleNum = slots[i].vehicleNumber;
          rfidUID = slots[i].bookedByUID;
          
          User* user = findUserByRFID(rfidUID);
          if (user != nullptr) {
            userEmail = user->email;
            slots[i].currentUserEmail = userEmail;
          }
          
          Serial.println("   Booking confirmed for: " + userName);
          slots[i].booked = false;
          slots[i].bookedBy = "";
          slots[i].bookedByUID = "";
        }
        
        logActivity("entry", userName, userEmail, slots[i].slotName, vehicleNum, rfidUID);
        totalEntries++;
        
        updateSlotInFirebase(i);
        saveToPreferences();
      }
      else if (slots[i].occupied && !isOccupied) {
        Serial.println("🚗 Vehicle EXITED Slot " + slots[i].slotName);
        
        String duration = getDurationString(slots[i].occupiedTime, millis());
        
        String exitUserEmail = slots[i].currentUserEmail;
        String exitUserName = "User";
        
        if (!exitUserEmail.isEmpty()) {
          User* user = findUserByEmail(exitUserEmail);
          if (user != nullptr) {
            exitUserName = user->name;
          }
        }
        
        logActivity("exit", exitUserName, exitUserEmail, slots[i].slotName, slots[i].vehicleNumber, "", duration);
        totalExits++;
        
        slots[i].occupied = false;
        slots[i].occupiedTime = 0;
        slots[i].vehicleNumber = "";
        slots[i].currentUserEmail = "";
        digitalWrite(slots[i].ledPin, LOW);
        
        updateSlotInFirebase(i);
        saveToPreferences();
      }
    }
    
    if (!slots[i].occupied && !slots[i].booked) {
      newAvailableSlots++;
    }
  }
  
  availableSlots = newAvailableSlots;
}

void checkRFID() {
  if (rfidDetected && (millis() - rfidDetectedTime > RFID_HOLD_TIME)) {
    rfidDetected = false;
    currentRFID = "";
  }
  
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return;
  }
  
  if (!mfrc522.PICC_ReadCardSerial()) {
    return;
  }
  
  String cardUID = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    cardUID += (mfrc522.uid.uidByte[i] < 0x10 ? "0" : "") + String(mfrc522.uid.uidByte[i], HEX);
  }
  cardUID.toUpperCase();
  
  if (cardUID != currentRFID) {
    currentRFID = cardUID;
    rfidDetected = true;
    rfidDetectedTime = millis();
    
    Serial.println("\n🎫 RFID DETECTED: " + cardUID);
    
    bool hasBooking = false;
    for (int i = 0; i < 4; i++) {
      if (slots[i].booked && slots[i].bookedByUID == cardUID) {
        hasBooking = true;
        handleRFIDAccess(cardUID);
        break;
      }
    }
    
    if (!hasBooking) {
      Serial.println("   ℹ️  No active booking");
      lcd.clear();
      lcd.print("RFID Detected!");
      lcd.setCursor(0, 1);
      lcd.print("ID:" + cardUID.substring(0, 12));
    }
  }
  
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}

void handleRFIDAccess(String cardUID) {
  int bookedSlot = -1;
  String userName = "";
  
  for (int i = 0; i < 4; i++) {
    if (slots[i].booked && slots[i].bookedByUID == cardUID) {
      bookedSlot = i;
      userName = slots[i].bookedBy;
      break;
    }
  }
  
  if (bookedSlot >= 0) {
    Serial.println("✅ ACCESS GRANTED: " + userName);
    
    lastUser = userName;
    
    lcd.clear();
    lcd.print("Welcome!");
    lcd.setCursor(0, 1);
    lcd.print(userName);
    
    controlGate(true); // OPENS INSTANTLY!
  }
}

void checkBookingTimeouts() {
  for (int i = 0; i < 4; i++) {
    if (slots[i].booked && !slots[i].occupied) {
      if (millis() - slots[i].bookingTime > 1800000) {
        Serial.println("⏰ Booking timeout: Slot " + slots[i].slotName);
        
        User* user = findUserByRFID(slots[i].bookedByUID);
        String userEmail = (user != nullptr) ? user->email : "";
        logActivity("timeout", slots[i].bookedBy, userEmail, slots[i].slotName, slots[i].vehicleNumber, slots[i].bookedByUID);
        
        slots[i].booked = false;
        slots[i].bookedBy = "";
        slots[i].bookedByUID = "";
        slots[i].bookingTime = 0;
        
        if (!slots[i].occupied) {
          digitalWrite(slots[i].ledPin, LOW);
        }
        
        updateSlotInFirebase(i);
        saveToPreferences();
      }
    }
  }
}

// ==================== ACTIVITY LOGGING ====================
void logActivity(String type, String userName, String userEmail, String slotName, String vehicleNumber, String rfidUID, String duration) {
  ActivityLog activity;
  activity.activityType = type;
  activity.userName = userName;
  activity.userEmail = userEmail;
  activity.slotName = slotName;
  activity.vehicleNumber = vehicleNumber;
  activity.rfidUID = rfidUID;
  activity.timestamp = millis();
  activity.duration = duration;
  
  activityHistory.push_back(activity);
  
  if (activityHistory.size() > MAX_HISTORY) {
    activityHistory.erase(activityHistory.begin());
  }
  
  saveActivityToFirebase(activity);
  
  Serial.println("📝 Activity logged: " + type + " - " + userName + " - Slot " + slotName);
}

void saveActivityToFirebase(const ActivityLog& activity) {
  if (WiFi.status() != WL_CONNECTED) return;
  
  HTTPClient http;
  String activityId = String(activity.timestamp);
  String url = String(DATABASE_URL) + "parking/history/" + activityId + ".json";
  
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  
  DynamicJsonDocument doc(512);
  doc["type"] = activity.activityType;
  doc["userName"] = activity.userName;
  doc["userEmail"] = activity.userEmail;
  doc["slot"] = activity.slotName;
  doc["vehicle"] = activity.vehicleNumber;
  doc["rfidUID"] = activity.rfidUID;
  doc["timestamp"] = activity.timestamp;
  doc["duration"] = activity.duration;
  doc["formattedTime"] = getFormattedTime(activity.timestamp);
  
  String json;
  serializeJson(doc, json);
  
  http.PUT(json);
  http.end();
}

String getFormattedTime(unsigned long timestamp) {
  unsigned long seconds = timestamp / 1000;
  unsigned long minutes = seconds / 60;
  unsigned long hours = minutes / 60;
  unsigned long days = hours / 24;
  
  hours = hours % 24;
  minutes = minutes % 60;
  seconds = seconds % 60;
  
  char buffer[50];
  sprintf(buffer, "%lud %02lu:%02lu:%02lu", days, hours, minutes, seconds);
  return String(buffer);
}

String getDurationString(unsigned long startTime, unsigned long endTime) {
  unsigned long duration = (endTime - startTime) / 1000;
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

void updateSlotInFirebase(int slotIndex) {
  if (WiFi.status() != WL_CONNECTED) return;
  
  HTTPClient http;
  String url = String(DATABASE_URL) + "parking/slots/" + slots[slotIndex].slotName + ".json";
  
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  
  DynamicJsonDocument doc(512);
  doc["occupied"] = slots[slotIndex].occupied;
  doc["booked"] = slots[slotIndex].booked;
  doc["bookedBy"] = slots[slotIndex].bookedBy;
  doc["vehicleNumber"] = slots[slotIndex].vehicleNumber;
  doc["currentUserEmail"] = slots[slotIndex].currentUserEmail;
  doc["timestamp"] = millis();
  
  String json;
  serializeJson(doc, json);
  
  http.PUT(json);
  http.end();
}

void updateFirebaseStatus() {
  if (WiFi.status() != WL_CONNECTED) return;
  
  HTTPClient http;
  String url = String(DATABASE_URL) + "parking/status.json";
  
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  
  DynamicJsonDocument doc(512);
  doc["availableSlots"] = availableSlots;
  doc["totalSlots"] = 4;
  doc["totalBookings"] = totalBookings;
  doc["totalEntries"] = totalEntries;
  doc["totalExits"] = totalExits;
  doc["gateOpen"] = gateOpen;
  doc["lastUpdate"] = millis();
  
  String json;
  serializeJson(doc, json);
  
  http.PUT(json);
  http.end();
}

void updateUserInFirebase(const User& user) {
  if (WiFi.status() != WL_CONNECTED) return;
  
  HTTPClient http;
  String url = String(DATABASE_URL) + "users/" + user.uid + ".json";
  
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  
  DynamicJsonDocument doc(512);
  doc["name"] = user.name;
  doc["email"] = user.email;
  doc["phone"] = user.phone;
  doc["vehicleNumber"] = user.vehicleNumber;
  doc["rfidUID"] = user.rfidUID;
  doc["password"] = user.password;
  doc["createdAt"] = millis();
  
  String json;
  serializeJson(doc, json);
  
  http.PUT(json);
  http.end();
}

User* findUserByEmail(String email) {
  for (size_t i = 0; i < users.size(); i++) {
    if (users[i].email == email) {
      return &users[i];
    }
  }
  return nullptr;
}

User* findUserByRFID(String rfidUID) {
  for (size_t i = 0; i < users.size(); i++) {
    if (users[i].rfidUID == rfidUID) {
      return &users[i];
    }
  }
  return nullptr;
}

void saveToPreferences() {
  preferences.putInt("totalBookings", totalBookings);
  preferences.putInt("totalEntries", totalEntries);
  preferences.putInt("totalExits", totalExits);
  preferences.putString("lastUser", lastUser);
  
  for (int i = 0; i < 4; i++) {
    String prefix = "slot" + String(i) + "_";
    preferences.putBool((prefix + "occupied").c_str(), slots[i].occupied);
    preferences.putBool((prefix + "booked").c_str(), slots[i].booked);
    preferences.putString((prefix + "bookedBy").c_str(), slots[i].bookedBy);
    preferences.putString((prefix + "bookedByUID").c_str(), slots[i].bookedByUID);
    preferences.putString((prefix + "currentUserEmail").c_str(), slots[i].currentUserEmail);
    preferences.putULong((prefix + "bookingTime").c_str(), slots[i].bookingTime);
  }
}

void loadFromPreferences() {
  totalBookings = preferences.getInt("totalBookings", 0);
  totalEntries = preferences.getInt("totalEntries", 0);
  totalExits = preferences.getInt("totalExits", 0);
  lastUser = preferences.getString("lastUser", "None");
  
  for (int i = 0; i < 4; i++) {
    String prefix = "slot" + String(i) + "_";
    slots[i].booked = preferences.getBool((prefix + "booked").c_str(), false);
    slots[i].bookedBy = preferences.getString((prefix + "bookedBy").c_str(), "");
    slots[i].bookedByUID = preferences.getString((prefix + "bookedByUID").c_str(), "");
    slots[i].currentUserEmail = preferences.getString((prefix + "currentUserEmail").c_str(), "");
    slots[i].bookingTime = preferences.getULong((prefix + "bookingTime").c_str(), 0);
  }
}

void setupWebServer() {
  server.on("/", handleHomePage);
  server.on("/login", handleLoginPage);
  server.on("/signup", handleSignupPage);
  server.on("/dashboard", handleUserDashboard);
  server.on("/admin", handleAdminDashboard);
  
  server.on("/api/register", HTTP_POST, handleAPIRegister);
  server.on("/api/login", HTTP_POST, handleAPILogin);
  server.on("/api/check-rfid", handleAPICheckRFID);
  server.on("/api/book-slot", HTTP_POST, handleAPIBookSlot);
  server.on("/api/cancel-booking", HTTP_POST, handleAPICancelBooking);
  server.on("/api/slots", handleAPISlots);
  server.on("/api/status", handleAPIStatus);
  server.on("/api/gate/open", handleGateOpen);
  server.on("/api/gate/close", handleGateClose);
  server.on("/api/admin/users", handleAPIAdminUsers);
  server.on("/api/admin/bookings", handleAPIAdminBookings);
  server.on("/api/history", handleAPIHistory);
  server.on("/api/user-history", handleAPIUserHistory);
  server.on("/api/get-profile", handleAPIGetUserProfile);
  server.on("/api/update-profile", HTTP_POST, handleAPIUpdateProfile);
  
  Serial.println("✅ Web server configured");
}

// ==================== HTML PAGES ====================
void handleHomePage() {
  String html = R"=====(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Smart Parking System</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:'Segoe UI',sans-serif;background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);min-height:100vh;display:flex;align-items:center;justify-content:center}
.container{background:white;border-radius:20px;box-shadow:0 20px 60px rgba(0,0,0,0.3);max-width:400px;width:90%;padding:40px;text-align:center}
.logo{font-size:4rem;margin-bottom:20px}
h1{color:#333;margin-bottom:10px;font-size:2rem}
p{color:#666;margin-bottom:30px}
.btn{display:block;width:100%;padding:15px;margin:10px 0;background:#667eea;color:white;text-decoration:none;border-radius:10px;font-weight:600;font-size:1rem;transition:all 0.3s;border:none;cursor:pointer}
.btn:hover{background:#5568d3;transform:translateY(-2px)}
.btn.secondary{background:#10b981}
.btn.secondary:hover{background:#059669}
.btn.admin{background:#ef4444}
.btn.admin:hover{background:#dc2626}
.status-bar{background:#f3f4f6;padding:15px;border-radius:10px;margin-top:30px}
.status-grid{display:grid;grid-template-columns:1fr 1fr;gap:10px;margin-top:10px}
.stat{background:white;padding:10px;border-radius:8px}
.stat-value{font-size:1.5rem;font-weight:700;color:#667eea}
.stat-label{font-size:0.75rem;color:#666;margin-top:5px}
</style>
</head>
<body>
<div class="container">
<div class="logo">🚗</div>
<h1>Smart Parking</h1>
<p>Book your parking slot in advance</p>
<a href="/login" class="btn">Login</a>
<a href="/signup" class="btn secondary">Sign Up</a>
<button class="btn admin" onclick="adminLogin()">Admin Dashboard</button>
<div class="status-bar">
<strong>🅿 Live Status</strong>
<div class="status-grid">
<div class="stat">
<div class="stat-value" id="availableSlots">-</div>
<div class="stat-label">Available</div>
</div>
<div class="stat">
<div class="stat-value">4</div>
<div class="stat-label">Total Slots</div>
</div>
</div>
</div>
</div>
<script>
function updateStatus(){
fetch('/api/status').then(r=>r.json()).then(data=>{
document.getElementById('availableSlots').textContent=data.availableSlots;
}).catch(e=>console.log(e));
}
updateStatus();
setInterval(updateStatus,2000);

function adminLogin() {
var pwd = prompt('Enter admin password:');
if (pwd === '0000') {
window.location.href = '/admin';
} else if (pwd !== null) {
alert('Incorrect password');
}
}
</script>
</body>
</html>
)=====" ;
  server.send(200, "text/html", html);
}

void handleSignupPage() {
  String html = R"=====(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Sign Up</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:'Segoe UI',sans-serif;background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);min-height:100vh;display:flex;align-items:center;justify-content:center;padding:20px}
.container{background:white;border-radius:20px;box-shadow:0 20px 60px rgba(0,0,0,0.3);max-width:500px;width:100%;padding:40px}
h1{text-align:center;color:#333;margin-bottom:30px}
.form-group{margin-bottom:20px}
label{display:block;color:#555;font-weight:600;margin-bottom:8px;font-size:0.9rem}
input,select{width:100%;padding:12px;border:2px solid #e5e7eb;border-radius:8px;font-size:1rem}
input:focus,select:focus{outline:none;border-color:#667eea}
.rfid-section{background:#f3f4f6;padding:20px;border-radius:10px;margin:20px 0;border:2px solid #ddd}
.rfid-section.detected{border-color:#10b981;background:#d1fae5}
.rfid-status{text-align:center;padding:20px;background:white;border-radius:8px;margin-top:10px}
.btn{width:100%;padding:15px;background:#667eea;color:white;border:none;border-radius:10px;font-size:1rem;font-weight:600;cursor:pointer}
.btn:hover:not(:disabled){background:#5568d3}
.btn:disabled{background:#9ca3af;cursor:not-allowed}
.back-link{text-align:center;margin-top:20px}
.back-link a{color:#667eea;text-decoration:none}
.pulse{animation:pulse 2s infinite}
@keyframes pulse{0%,100%{opacity:1}50%{opacity:0.5}}
</style>
</head>
<body>
<div class="container">
<h1>📝 Create Account</h1>
<form id="signupForm">
<div class="form-group">
<label>Full Name *</label>
<input type="text" id="name" required>
</div>
<div class="form-group">
<label>Email *</label>
<input type="email" id="email" required>
</div>
<div class="form-group">
<label>Phone Number *</label>
<input type="tel" id="phone" required>
</div>
<div class="form-group">
<label>Password *</label>
<input type="password" id="password" required>
</div>
<div class="form-group">
<label>Vehicle Type *</label>
<select id="vehicleType" required>
<option value="">Select</option>
<option value="2wheeler">Two Wheeler</option>
<option value="4wheeler">Four Wheeler</option>
</select>
</div>
<div class="form-group">
<label>Vehicle Number *</label>
<input type="text" id="vehicleNumber" required>
</div>
<div class="rfid-section" id="rfidSection">
<strong>🎫 RFID Card Registration</strong>
<div class="rfid-status">
<div id="rfidStatus" class="pulse">⏳ Scanning...</div>
<div id="rfidUID" style="display:none;margin-top:10px;font-weight:700;color:#10b981"></div>
</div>
<p style="margin-top:10px;font-size:0.85rem;color:#666;text-align:center">
<strong>Place RFID card near reader</strong>
</p>
</div>
<button type="submit" class="btn" id="submitBtn" disabled>Create Account</button>
</form>
<div class="back-link"><a href="/">← Back</a></div>
</div>
<script>
let rfidScanned=false;
let rfidUID='';

function checkRFID(){
if(!rfidScanned){
fetch('/api/check-rfid').then(r=>r.json()).then(data=>{
if(data.success && data.rfid){
rfidUID=data.rfid;
rfidScanned=true;
document.getElementById('rfidStatus').textContent='✅ Card Detected!';
document.getElementById('rfidStatus').classList.remove('pulse');
document.getElementById('rfidUID').textContent='ID: '+rfidUID;
document.getElementById('rfidUID').style.display='block';
document.getElementById('rfidSection').classList.add('detected');
document.getElementById('submitBtn').disabled=false;
}
});
}
}

setInterval(checkRFID,1000);
checkRFID();

document.getElementById('signupForm').addEventListener('submit',function(e){
e.preventDefault();
if(!rfidScanned){alert('Please scan RFID!');return;}
document.getElementById('submitBtn').disabled=true;

fetch('/api/register',{
method:'POST',
headers:{'Content-Type':'application/json'},
body:JSON.stringify({
name:document.getElementById('name').value,
email:document.getElementById('email').value,
phone:document.getElementById('phone').value,
password:document.getElementById('password').value,
vehicleType:document.getElementById('vehicleType').value,
vehicleNumber:document.getElementById('vehicleNumber').value,
rfidUID:rfidUID
})
}).then(r=>r.json()).then(data=>{
if(data.success){
alert('Account created!');
window.location.href='/login';
}else{
alert(data.message);
document.getElementById('submitBtn').disabled=false;
}
});
});
</script>
</body>
</html>
)=====" ;
  server.send(200, "text/html", html);
}

void handleLoginPage() {
  String html = R"=====(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Login</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:'Segoe UI',sans-serif;background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);min-height:100vh;display:flex;align-items:center;justify-content:center}
.container{background:white;border-radius:20px;box-shadow:0 20px 60px rgba(0,0,0,0.3);max-width:400px;width:90%;padding:40px}
h1{text-align:center;color:#333;margin-bottom:30px}
.form-group{margin-bottom:20px}
label{display:block;color:#555;font-weight:600;margin-bottom:8px}
input{width:100%;padding:12px;border:2px solid #e5e7eb;border-radius:8px;font-size:1rem}
input:focus{outline:none;border-color:#667eea}
.btn{width:100%;padding:15px;background:#667eea;color:white;border:none;border-radius:10px;font-size:1rem;font-weight:600;cursor:pointer;margin-top:10px}
.btn:hover{background:#5568d3}
.links{text-align:center;margin-top:20px}
.links a{color:#667eea;text-decoration:none;margin:0 10px}
</style>
</head>
<body>
<div class="container">
<h1>🔐 Login</h1>
<form id="loginForm">
<div class="form-group">
<label>Email</label>
<input type="email" id="email" required>
</div>
<div class="form-group">
<label>Password</label>
<input type="password" id="password" required>
</div>
<button type="submit" class="btn">Login</button>
</form>
<div class="links">
<a href="/signup">Sign Up</a> | <a href="/">Home</a>
</div>
</div>
<script>
document.getElementById('loginForm').addEventListener('submit',function(e){
e.preventDefault();
fetch('/api/login',{
method:'POST',
headers:{'Content-Type':'application/json'},
body:JSON.stringify({
email:document.getElementById('email').value,
password:document.getElementById('password').value
})
}).then(r=>r.json()).then(result=>{
if(result.success){
localStorage.setItem('userEmail',result.email);
localStorage.setItem('userName',result.name);
window.location.href='/dashboard';
}else{
alert('Login failed');
}
});
});
</script>
</body>
</html>
)=====" ;
  server.send(200, "text/html", html);
}

void handleUserDashboard() {
  String html = R"=====(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>My Dashboard</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:'Segoe UI',sans-serif;background:#f5f7fa}
.header{background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);color:white;padding:20px;text-align:center;position:relative}
.back-home{position:absolute;left:20px;top:20px;background:rgba(255,255,255,0.2);padding:10px 20px;border-radius:8px;color:white;text-decoration:none;font-weight:600}
.back-home:hover{background:rgba(255,255,255,0.3)}
.container{max-width:1200px;margin:20px auto;padding:20px}
.welcome{background:white;padding:20px;border-radius:12px;margin-bottom:20px}
.slots-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(250px,1fr));gap:20px;margin-bottom:30px}
.slot-card{background:white;border-radius:12px;padding:20px;box-shadow:0 2px 10px rgba(0,0,0,0.1);text-align:center}
.slot-card.available{border:3px solid #10b981}
.slot-card.occupied{border:3px solid #ef4444}
.slot-card.booked{border:3px solid #f59e0b}
.slot-icon{font-size:3rem;margin-bottom:10px}
.btn{padding:10px 20px;background:#667eea;color:white;border:none;border-radius:8px;cursor:pointer;font-weight:600;margin-top:10px}
.btn.danger{background:#ef4444}
.history-section,.profile-section{background:white;padding:25px;border-radius:12px;margin-top:30px;box-shadow:0 2px 10px rgba(0,0,0,0.1)}
table{width:100%;border-collapse:collapse;margin-top:15px}
th,td{padding:12px;text-align:left;border-bottom:1px solid #e5e7eb}
th{background:#f9fafb;font-weight:600}
.badge{padding:4px 12px;border-radius:12px;font-size:0.85rem;font-weight:600}
.badge.entry{background:#d1fae5;color:#065f46}
.badge.exit{background:#fecaca;color:#991b1b}
.badge.booking{background:#dbeafe;color:#1e40af}
.form-group{margin-bottom:20px}
label{display:block;color:#555;font-weight:600;margin-bottom:8px;font-size:0.9rem}
input,select{width:100%;padding:12px;border:2px solid #e5e7eb;border-radius:8px;font-size:1rem}
input:focus,select:focus{outline:none;border-color:#667eea}
.rfid-section{background:#f3f4f6;padding:20px;border-radius:10px;margin:20px 0;border:2px solid #ddd}
.rfid-section.detected{border-color:#10b981;background:#d1fae5}
.rfid-status{text-align:center;padding:20px;background:white;border-radius:8px;margin-top:10px}
.btn-update{background:#10b981}
.btn-update:hover{background:#059669}
.pulse{animation:pulse 2s infinite}
@keyframes pulse{0%,100%{opacity:1}50%{opacity:0.5}}
</style>
</head>
<body>
<div class="header">
<a href="/" class="back-home">🏠 Home</a>
<h1>🚗 My Dashboard</h1>
<p id="userName">Welcome!</p>
</div>
<div class="container">
<div class="welcome">
<h2>Select a Parking Slot</h2>
<p>Choose an available slot to book</p>
</div>
<div class="slots-grid" id="slotsGrid"></div>

<div class="history-section">
<h2>📊 My Activity History</h2>
<p style="color:#666;margin-bottom:15px">Your entries and exits record</p>
<table>
<thead>
<tr><th>Date/Time</th><th>Type</th><th>Slot</th><th>Duration</th></tr>
</thead>
<tbody id="historyTable">
<tr><td colspan="4" style="text-align:center;color:#666">Loading...</td></tr>
</tbody>
</table>
</div>

<div class="profile-section">
<h2>👤 My Profile</h2>
<p style="color:#666;margin-bottom:20px">Update your account details</p>
<form id="profileForm">
<div class="form-group">
<label>Full Name</label>
<input type="text" id="profileName" required>
</div>
<div class="form-group">
<label>Email (Cannot be changed)</label>
<input type="email" id="profileEmail" readonly style="background:#f3f4f6">
</div>
<div class="form-group">
<label>Phone Number</label>
<input type="tel" id="profilePhone" required>
</div>
<div class="form-group">
<label>Vehicle Number</label>
<input type="text" id="profileVehicle" required>
</div>
<div class="form-group">
<label>New Password (Leave blank to keep current)</label>
<input type="password" id="profilePassword" placeholder="Enter new password">
</div>
<div class="rfid-section" id="rfidSection" style="display:none">
<strong>🎫 Update RFID Card (Optional)</strong>
<div class="rfid-status">
<div id="rfidStatus" class="pulse">⏳ Scanning...</div>
<div id="rfidUID" style="display:none;margin-top:10px;font-weight:700;color:#10b981"></div>
</div>
<p style="margin-top:10px;font-size:0.85rem;color:#666;text-align:center">
Place new RFID card near reader (optional)
</p>
</div>
<div style="display:flex;gap:10px">
<button type="button" class="btn" onclick="toggleRFIDUpdate()">Change RFID Card</button>
<button type="submit" class="btn btn-update">Update Profile</button>
</div>
</form>
</div>
</div>
<script>
const userName=localStorage.getItem('userName')||'User';
const userEmail=localStorage.getItem('userEmail')||'';
document.getElementById('userName').textContent='Welcome, '+userName;

let rfidUpdateMode=false;
let newRFID='';

function loadProfile(){
fetch('/api/get-profile?email='+encodeURIComponent(userEmail))
.then(r=>r.json())
.then(data=>{
if(data.success){
document.getElementById('profileName').value=data.name;
document.getElementById('profileEmail').value=data.email;
document.getElementById('profilePhone').value=data.phone;
document.getElementById('profileVehicle').value=data.vehicleNumber;
}
});
}

function toggleRFIDUpdate(){
rfidUpdateMode=!rfidUpdateMode;
document.getElementById('rfidSection').style.display=rfidUpdateMode?'block':'none';
if(!rfidUpdateMode){
newRFID='';
document.getElementById('rfidStatus').textContent='⏳ Scanning...';
document.getElementById('rfidUID').style.display='none';
document.getElementById('rfidSection').classList.remove('detected');
}
}

function checkRFID(){
if(rfidUpdateMode && !newRFID){
fetch('/api/check-rfid').then(r=>r.json()).then(data=>{
if(data.success && data.rfid){
newRFID=data.rfid;
document.getElementById('rfidStatus').textContent='✅ New Card Detected!';
document.getElementById('rfidStatus').classList.remove('pulse');
document.getElementById('rfidUID').textContent='New ID: '+newRFID;
document.getElementById('rfidUID').style.display='block';
document.getElementById('rfidSection').classList.add('detected');
}
});
}
}

setInterval(checkRFID,1000);

document.getElementById('profileForm').addEventListener('submit',function(e){
e.preventDefault();
const updateData={
email:userEmail,
name:document.getElementById('profileName').value,
phone:document.getElementById('profilePhone').value,
vehicleNumber:document.getElementById('profileVehicle').value,
password:document.getElementById('profilePassword').value,
rfidUID:newRFID
};

fetch('/api/update-profile',{
method:'POST',
headers:{'Content-Type':'application/json'},
body:JSON.stringify(updateData)
}).then(r=>r.json()).then(data=>{
if(data.success){
alert('Profile updated successfully!');
localStorage.setItem('userName',updateData.name);
document.getElementById('userName').textContent='Welcome, '+updateData.name;
document.getElementById('profilePassword').value='';
toggleRFIDUpdate();
}else{
alert('Update failed: '+data.message);
}
});
});

function refreshSlots(){
fetch('/api/slots').then(r=>r.json()).then(data=>{
const grid=document.getElementById('slotsGrid');
grid.innerHTML=data.slots.map(slot=>{
const statusClass=slot.occupied?'occupied':(slot.booked?'booked':'available');
const statusText=slot.occupied?'Occupied':(slot.booked?'Booked':'Available');
const statusIcon=slot.occupied?'🔴':(slot.booked?'🟡':'🟢');
let btnHtml='';
if(!slot.occupied&&!slot.booked){
btnHtml='<button class="btn" onclick="bookSlot(\''+slot.name+'\')">Book</button>';
}
if(slot.booked){
btnHtml='<button class="btn danger" onclick="cancelBooking(\''+slot.name+'\')">Cancel</button>';
}
return '<div class="slot-card '+statusClass+'">'+
'<div class="slot-icon">🅿</div>'+
'<h3>Slot '+slot.name+'</h3>'+
'<p>'+statusIcon+' '+statusText+'</p>'+
btnHtml+'</div>';
}).join('');
});
}

function refreshHistory(){
fetch('/api/user-history?email='+encodeURIComponent(userEmail))
.then(r=>r.json())
.then(data=>{
const tbody=document.getElementById('historyTable');
const activities=data.activities||[];
if(activities.length===0){
tbody.innerHTML='<tr><td colspan="4" style="text-align:center;color:#666">No activity yet</td></tr>';
return;
}
tbody.innerHTML=activities.map(a=>{
const icon=a.type==='entry'?'🚗➡️':'🚗⬅️';
const badgeClass=a.type;
return '<tr>'+
'<td>'+a.formattedTime+'</td>'+
'<td><span class="badge '+badgeClass+'">'+icon+' '+a.type.toUpperCase()+'</span></td>'+
'<td>Slot '+a.slot+'</td>'+
'<td>'+(a.duration||'-')+'</td>'+
'</tr>';
}).join('');
});
}

function bookSlot(slotName){
fetch('/api/book-slot',{
method:'POST',
headers:{'Content-Type':'application/json'},
body:JSON.stringify({slot:slotName,userEmail:userEmail})
}).then(r=>r.json()).then(data=>{
if(data.success){
alert('Slot '+slotName+' booked! Use your RFID card at the gate.');
refreshSlots();
}else{
alert('Booking failed: '+data.message);
}
});
}

function cancelBooking(slotName){
if(confirm('Cancel booking?')){
fetch('/api/cancel-booking',{
method:'POST',
headers:{'Content-Type':'application/json'},
body:JSON.stringify({slot:slotName,userEmail:userEmail})
}).then(r=>r.json()).then(data=>{
alert(data.message);
refreshSlots();
});
}
}

setInterval(refreshSlots,3000);
setInterval(refreshHistory,5000);
loadProfile();
refreshSlots();
refreshHistory();
</script>
</body>
</html>
)=====" ;
  server.send(200, "text/html", html);
}

void handleAdminDashboard() {
  String html = R"=====(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Admin Dashboard</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:'Segoe UI',sans-serif;background:#f5f7fa}
.header{background:linear-gradient(135deg,#ef4444 0%,#dc2626 100%);color:white;padding:20px;text-align:center;position:relative}
.back-home{position:absolute;left:20px;top:20px;background:rgba(255,255,255,0.2);padding:10px 20px;border-radius:8px;color:white;text-decoration:none;font-weight:600}
.back-home:hover{background:rgba(255,255,255,0.3)}
.container{max-width:1400px;margin:20px auto;padding:20px}
.stats{display:grid;grid-template-columns:repeat(auto-fit,minmax(200px,1fr));gap:20px;margin-bottom:30px}
.stat-card{background:white;padding:25px;border-radius:12px;text-align:center;box-shadow:0 2px 10px rgba(0,0,0,0.1)}
.stat-value{font-size:2.5rem;font-weight:700;color:#ef4444}
.stat-label{color:#666;margin-top:10px}
.section{background:white;padding:25px;border-radius:12px;margin-bottom:20px;box-shadow:0 2px 10px rgba(0,0,0,0.1)}
table{width:100%;border-collapse:collapse}
th,td{padding:12px;text-align:left;border-bottom:1px solid #e5e7eb}
th{background:#f9fafb;font-weight:600}
.btn{padding:8px 16px;background:#667eea;color:white;border:none;border-radius:6px;cursor:pointer;margin:0 5px}
.btn.danger{background:#ef4444}
.controls{display:flex;gap:10px;margin-bottom:20px}
.badge{padding:4px 12px;border-radius:12px;font-size:0.85rem;font-weight:600}
.badge.entry{background:#d1fae5;color:#065f46}
.badge.exit{background:#fecaca;color:#991b1b}
.badge.booking{background:#dbeafe;color:#1e40af}
.badge.cancel{background:#fef3c7;color:#92400e}
.filter-btn{padding:8px 16px;margin:5px;background:#667eea;color:white;border:none;border-radius:6px;cursor:pointer}
.filter-btn.active{background:#ef4444}
.slots-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(250px,1fr));gap:20px;margin-bottom:30px}
.slot-card{background:white;border-radius:12px;padding:20px;box-shadow:0 2px 10px rgba(0,0,0,0.1);text-align:center}
.slot-card.available{border:3px solid #10b981}
.slot-card.occupied{border:3px solid #ef4444}
.slot-card.booked{border:3px solid #f59e0b}
.slot-icon{font-size:3rem;margin-bottom:10px}
.slot-details{margin-top:10px;font-size:0.9rem;color:#666}
</style>
</head>
<body>
<div class="header">
<a href="/" class="back-home">🏠 Home</a>
<h1>👨‍💼 Admin Dashboard</h1>
<p>Smart Parking Management</p>
</div>
<div class="container">
<div class="stats">
<div class="stat-card">
<div class="stat-value" id="totalUsers">0</div>
<div class="stat-label">Total Users</div>
</div>
<div class="stat-card">
<div class="stat-value" id="activeBookings">0</div>
<div class="stat-label">Active Bookings</div>
</div>
<div class="stat-card">
<div class="stat-value" id="totalEntriesAdmin">0</div>
<div class="stat-label">Total Entries</div>
</div>
<div class="stat-card">
<div class="stat-value" id="totalExitsAdmin">0</div>
<div class="stat-label">Total Exits</div>
</div>
</div>

<div class="section">
<h2>🅿 Parking Slots Status</h2>
<p style="color:#666;margin-bottom:15px">Real-time view of all parking slots</p>
<div class="slots-grid" id="adminSlotsGrid"></div>
</div>

<div class="section">
<h2>🚪 Gate Control</h2>
<div class="controls">
<button class="btn" onclick="controlGate('open')">Open Gate</button>
<button class="btn danger" onclick="controlGate('close')">Close Gate</button>
<span id="gateStatus" style="margin-left:20px;font-weight:600"></span>
</div>
</div>
<div class="section">
<h2>👥 Registered Users</h2>
<table>
<thead><tr><th>Name</th><th>Email</th><th>Vehicle</th><th>RFID</th><th>Phone</th></tr></thead>
<tbody id="usersTable"><tr><td colspan="5" style="text-align:center">Loading...</td></tr></tbody>
</table>
</div>
<div class="section">
<h2>📅 Current Bookings</h2>
<table>
<thead><tr><th>Slot</th><th>User</th><th>Vehicle</th><th>Status</th></tr></thead>
<tbody id="bookingsTable"><tr><td colspan="4" style="text-align:center">Loading...</td></tr></tbody>
</table>
</div>

<div class="section">
<h2>📊 Complete Activity History</h2>
<p style="color:#666;margin-bottom:15px">All user entries and exits</p>
<div style="margin:15px 0">
<button class="filter-btn active" onclick="filterHistory('all')">All</button>
<button class="filter-btn" onclick="filterHistory('entry')">Entries</button>
<button class="filter-btn" onclick="filterHistory('exit')">Exits</button>
<button class="filter-btn" onclick="filterHistory('booking')">Bookings</button>
</div>
<table>
<thead><tr><th>Time</th><th>Type</th><th>User</th><th>Slot</th><th>Vehicle</th><th>Duration</th></tr></thead>
<tbody id="historyTable"><tr><td colspan="6" style="text-align:center">Loading...</td></tr></tbody>
</table>
</div>
</div>
<script>
let allHistory=[];
let currentFilter='all';

function refreshAdminSlots(){
fetch('/api/slots').then(r=>r.json()).then(data=>{
const grid=document.getElementById('adminSlotsGrid');
grid.innerHTML=data.slots.map(slot=>{
const statusClass=slot.occupied?'occupied':(slot.booked?'booked':'available');
const statusText=slot.occupied?'Occupied':(slot.booked?'Booked':'Available');
const statusIcon=slot.occupied?'🔴':(slot.booked?'🟡':'🟢');

let details='';
if(slot.booked){
details='<div class="slot-details">Booked by: '+slot.occupiedBy+'<br>Vehicle: '+slot.vehicleNumber+'</div>';
}else if(slot.occupied){
details='<div class="slot-details">Currently Occupied<br>Vehicle: '+slot.vehicleNumber+'</div>';
}else{
details='<div class="slot-details">Available for booking</div>';
}

return '<div class="slot-card '+statusClass+'">'+
'<div class="slot-icon">🅿</div>'+
'<h3>Slot '+slot.name+'</h3>'+
'<p>'+statusIcon+' '+statusText+'</p>'+
details+
'</div>';
}).join('');
});
}

function refreshAdmin(){
refreshAdminSlots();

fetch('/api/admin/users').then(r=>r.json()).then(data=>{
document.getElementById('totalUsers').textContent=data.users?data.users.length:0;
const tbody=document.getElementById('usersTable');
if(data.users&&data.users.length>0){
tbody.innerHTML=data.users.map(u=>
'<tr><td>'+u.name+'</td><td>'+u.email+'</td><td>'+u.vehicleNumber+'</td><td>'+u.rfidUID+'</td><td>'+u.phone+'</td></tr>'
).join('');
}else{
tbody.innerHTML='<tr><td colspan="5" style="text-align:center">No users</td></tr>';
}
});

fetch('/api/admin/bookings').then(r=>r.json()).then(data=>{
document.getElementById('activeBookings').textContent=data.bookings?data.bookings.length:0;
const tbody=document.getElementById('bookingsTable');
if(data.bookings&&data.bookings.length>0){
tbody.innerHTML=data.bookings.map(b=>
'<tr><td>Slot '+b.slot+'</td><td>'+b.userName+'</td><td>'+b.vehicle+'</td><td>'+b.status+'</td></tr>'
).join('');
}else{
tbody.innerHTML='<tr><td colspan="4" style="text-align:center">No bookings</td></tr>';
}
});

fetch('/api/status').then(r=>r.json()).then(data=>{
document.getElementById('totalEntriesAdmin').textContent=data.totalEntries||0;
document.getElementById('totalExitsAdmin').textContent=data.totalExits||0;
document.getElementById('gateStatus').textContent='Gate: '+(data.gateOpen?'OPEN':'CLOSED');
});

fetch('/api/history').then(r=>r.json()).then(data=>{
allHistory=data.activities||[];
displayHistory();
});
}

function displayHistory(){
const tbody=document.getElementById('historyTable');
let filtered=currentFilter==='all'?allHistory:allHistory.filter(a=>a.type===currentFilter);

if(filtered.length===0){
tbody.innerHTML='<tr><td colspan="6" style="text-align:center;color:#666">No activities</td></tr>';
return;
}

tbody.innerHTML=filtered.map(a=>{
const icon=a.type==='entry'?'🚗➡️':a.type==='exit'?'🚗⬅️':a.type==='booking'?'📅':'❌';
return '<tr>'+
'<td>'+a.formattedTime+'</td>'+
'<td><span class="badge '+a.type+'">'+icon+' '+a.type.toUpperCase()+'</span></td>'+
'<td>'+a.userName+'</td>'+
'<td>Slot '+a.slot+'</td>'+
'<td>'+(a.vehicle||'-')+'</td>'+
'<td>'+(a.duration||'-')+'</td>'+
'</tr>';
}).join('');
}

function filterHistory(type){
currentFilter=type;
document.querySelectorAll('.filter-btn').forEach(btn=>btn.classList.remove('active'));
event.target.classList.add('active');
displayHistory();
}

function controlGate(action){
fetch('/api/gate/'+action).then(r=>r.json()).then(data=>{
alert(data.message);
refreshAdmin();
});
}

setInterval(refreshAdmin,5000);
refreshAdmin();
</script>
</body>
</html>
)=====" ;
  server.send(200, "text/html", html);
}

// ==================== API HANDLERS ====================
  void handleAPICheckRFID() {
    DynamicJsonDocument doc(128);
    doc["success"] = rfidDetected;
    doc["timestamp"] = millis();
    if (rfidDetected) {
      doc["rfid"] = currentRFID;
    }
    
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
  }

  void handleAPIRegister() {
    if (!server.hasArg("plain")) {
      server.send(400, "application/json", "{\"success\":false}");
      return;
    }
    
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, server.arg("plain"));
    
    String name = doc["name"].as<String>();
    String email = doc["email"].as<String>();
    String phone = doc["phone"].as<String>();
    String password = doc["password"].as<String>();
    String vehicleNumber = doc["vehicleNumber"].as<String>();
    String rfidUID = doc["rfidUID"].as<String>();
    
    if (name.isEmpty() || email.isEmpty() || rfidUID.isEmpty()) {
      server.send(400, "application/json", "{\"success\":false}");
      return;
    }
    
    if (findUserByEmail(email) != nullptr) {
      server.send(409, "application/json", "{\"success\":false,\"message\":\"Email exists\"}");
      return;
    }
    
    if (findUserByRFID(rfidUID) != nullptr) {
      server.send(409, "application/json", "{\"success\":false,\"message\":\"RFID exists\"}");
      return;
    }
    
    User newUser;
    newUser.uid = "user_" + String(millis());
    newUser.name = name;
    newUser.email = email;
    newUser.phone = phone;
    newUser.vehicleNumber = vehicleNumber;
    newUser.rfidUID = rfidUID;
    newUser.password = password;
    
    users.push_back(newUser);
    updateUserInFirebase(newUser);
    
    rfidDetected = false;
    currentRFID = "";
    
    Serial.println("✅ New user: " + name);
    
    server.send(200, "application/json", "{\"success\":true}");
  }

  void handleAPILogin() {
    if (!server.hasArg("plain")) {
      server.send(400, "application/json", "{\"success\":false}");
      return;
    }
    
    DynamicJsonDocument doc(256);
    deserializeJson(doc, server.arg("plain"));
    
    String email = doc["email"].as<String>();
    
    User* user = findUserByEmail(email);
    if (user != nullptr) {
      DynamicJsonDocument response(256);
      response["success"] = true;
      response["email"] = user->email;
      response["name"] = user->name;
      
      String jsonResponse;
      serializeJson(response, jsonResponse);
      server.send(200, "application/json", jsonResponse);
    } else {
      server.send(401, "application/json", "{\"success\":false}");
    }
  }

  void handleAPIGetUserProfile() {
    if (!server.hasArg("email")) {
      server.send(400, "application/json", "{\"success\":false}");
      return;
    }
    
    String email = server.arg("email");
    User* user = findUserByEmail(email);
    
    if (user != nullptr) {
      DynamicJsonDocument doc(512);
      doc["success"] = true;
      doc["name"] = user->name;
      doc["email"] = user->email;
      doc["phone"] = user->phone;
      doc["vehicleNumber"] = user->vehicleNumber;
      doc["rfidUID"] = user->rfidUID;
      
      String response;
      serializeJson(doc, response);
      server.send(200, "application/json", response);
    } else {
      server.send(404, "application/json", "{\"success\":false}");
    }
  }

  void handleAPIUpdateProfile() {
    if (!server.hasArg("plain")) {
      server.send(400, "application/json", "{\"success\":false}");
      return;
    }
    
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, server.arg("plain"));
    
    String email = doc["email"].as<String>();
    User* user = findUserByEmail(email);
    
    if (user == nullptr) {
      server.send(404, "application/json", "{\"success\":false,\"message\":\"User not found\"}");
      return;
    }
    
    user->name = doc["name"].as<String>();
    user->phone = doc["phone"].as<String>();
    user->vehicleNumber = doc["vehicleNumber"].as<String>();
    
    String newPassword = doc["password"].as<String>();
    if (!newPassword.isEmpty()) {
      user->password = newPassword;
    }
    
    String newRFID = doc["rfidUID"].as<String>();
    if (!newRFID.isEmpty()) {
      User* existingRFID = findUserByRFID(newRFID);
      if (existingRFID != nullptr && existingRFID->email != email) {
        server.send(409, "application/json", "{\"success\":false,\"message\":\"RFID already in use\"}");
        return;
      }
      user->rfidUID = newRFID;
    }
    
    updateUserInFirebase(*user);
    
    Serial.println("✅ Profile updated: " + user->name);
    
    server.send(200, "application/json", "{\"success\":true,\"message\":\"Profile updated\"}");
  }

  void handleAPIBookSlot() {
    if (!server.hasArg("plain")) {
      server.send(400, "application/json", "{\"success\":false}");
      return;
    }
    
    DynamicJsonDocument doc(256);
    deserializeJson(doc, server.arg("plain"));
    
    String slotName = doc["slot"].as<String>();
    String userEmail = doc["userEmail"].as<String>();
    
    User* user = findUserByEmail(userEmail);
    if (user == nullptr) {
      server.send(404, "application/json", "{\"success\":false}");
      return;
    }
    
    int slotIndex = -1;
    for (int i = 0; i < 4; i++) {
      if (slots[i].slotName == slotName) {
        slotIndex = i;
        break;
      }
    }
    
    if (slotIndex == -1 || slots[slotIndex].occupied || slots[slotIndex].booked) {
      server.send(409, "application/json", "{\"success\":false,\"message\":\"Slot not available\"}");
      return;
    }
    
    for (int i = 0; i < 4; i++) {
      if (slots[i].booked && slots[i].bookedByUID == user->rfidUID) {
        server.send(409, "application/json", "{\"success\":false,\"message\":\"Already have booking\"}");
        return;
      }
    }
    
    slots[slotIndex].booked = true;
    slots[slotIndex].bookedBy = user->name;
    slots[slotIndex].bookedByUID = user->rfidUID;
    slots[slotIndex].vehicleNumber = user->vehicleNumber;
    slots[slotIndex].bookingTime = millis();
    
    digitalWrite(slots[slotIndex].ledPin, HIGH);
    
    totalBookings++;
    
    logActivity("booking", user->name, user->email, slotName, user->vehicleNumber, user->rfidUID);
    
    Serial.println("📅 BOOKING: " + user->name);
    
    updateSlotInFirebase(slotIndex);
    saveToPreferences();
    
    server.send(200, "application/json", "{\"success\":true}");
  }

  void handleAPICancelBooking() {
    if (!server.hasArg("plain")) {
      server.send(400, "application/json", "{\"success\":false}");
      return;
    }
    
    DynamicJsonDocument doc(256);
    deserializeJson(doc, server.arg("plain"));
    
    String slotName = doc["slot"].as<String>();
    String userEmail = doc["userEmail"].as<String>();
    
    int slotIndex = -1;
    for (int i = 0; i < 4; i++) {
      if (slots[i].slotName == slotName) {
        slotIndex = i;
        break;
      }
    }
    
    if (slotIndex == -1 || !slots[slotIndex].booked) {
      server.send(400, "application/json", "{\"success\":false}");
      return;
    }
    
    logActivity("cancel", slots[slotIndex].bookedBy, userEmail, slotName, slots[slotIndex].vehicleNumber, slots[slotIndex].bookedByUID);
    
    slots[slotIndex].booked = false;
    slots[slotIndex].bookedBy = "";
    slots[slotIndex].bookedByUID = "";
    slots[slotIndex].bookingTime = 0;
    slots[slotIndex].vehicleNumber = "";
    
    if (!slots[slotIndex].occupied) {
      digitalWrite(slots[slotIndex].ledPin, LOW);
    }
    
    updateSlotInFirebase(slotIndex);
    saveToPreferences();
    
    server.send(200, "application/json", "{\"success\":true,\"message\":\"Booking cancelled\"}");
  }

  void handleAPISlots() {
    DynamicJsonDocument doc(1024);
    JsonArray slotsArray = doc.createNestedArray("slots");
    
    for (int i = 0; i < 4; i++) {
      JsonObject slotObj = slotsArray.createNestedObject();
      slotObj["name"] = slots[i].slotName;
      slotObj["occupied"] = slots[i].occupied;
      slotObj["booked"] = slots[i].booked;
      slotObj["occupiedBy"] = slots[i].bookedBy;
      slotObj["vehicleNumber"] = slots[i].vehicleNumber;
    }
    
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
  }

  void handleAPIStatus() {
    DynamicJsonDocument doc(512);
    doc["availableSlots"] = availableSlots;
    doc["totalSlots"] = 4;
    doc["totalBookings"] = totalBookings;
    doc["totalEntries"] = totalEntries;
    doc["totalExits"] = totalExits;
    doc["lastUser"] = lastUser;
    doc["gateOpen"] = gateOpen;
    doc["uptime"] = (millis() - systemStartTime) / 1000;
    doc["totalUsers"] = users.size();
    
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
  }

  void handleAPIHistory() {
    DynamicJsonDocument doc(4096);
    JsonArray activitiesArray = doc.createNestedArray("activities");
    
    for (const ActivityLog& activity : activityHistory) {
      JsonObject activityObj = activitiesArray.createNestedObject();
      activityObj["type"] = activity.activityType;
      activityObj["userName"] = activity.userName;
      activityObj["userEmail"] = activity.userEmail;
      activityObj["slot"] = activity.slotName;
      activityObj["vehicle"] = activity.vehicleNumber;
      activityObj["duration"] = activity.duration;
      activityObj["formattedTime"] = getFormattedTime(activity.timestamp);
    }
    
    doc["totalEntries"] = totalEntries;
    doc["totalExits"] = totalExits;
    doc["totalBookings"] = totalBookings;
    
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
  }

  void handleAPIUserHistory() {
    if (!server.hasArg("email")) {
      server.send(400, "application/json", "{\"success\":false}");
      return;
    }
    
    String userEmail = server.arg("email");
    
    DynamicJsonDocument doc(2048);
    JsonArray activitiesArray = doc.createNestedArray("activities");
    
    for (const ActivityLog& activity : activityHistory) {
      if (activity.userEmail == userEmail) {
        JsonObject activityObj = activitiesArray.createNestedObject();
        activityObj["type"] = activity.activityType;
        activityObj["slot"] = activity.slotName;
        activityObj["duration"] = activity.duration;
        activityObj["formattedTime"] = getFormattedTime(activity.timestamp);
      }
    }
    
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
  }

  void handleAPIAdminUsers() {
    DynamicJsonDocument doc(2048);
    JsonArray usersArray = doc.createNestedArray("users");
    
    for (const User& user : users) {
      JsonObject userObj = usersArray.createNestedObject();
      userObj["name"] = user.name;
      userObj["email"] = user.email;
      userObj["phone"] = user.phone;
      userObj["vehicleNumber"] = user.vehicleNumber;
      userObj["rfidUID"] = user.rfidUID;
    }
    
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
  }

  void handleAPIAdminBookings() {
    DynamicJsonDocument doc(1024);
    JsonArray bookingsArray = doc.createNestedArray("bookings");
    
    for (int i = 0; i < 4; i++) {
      if (slots[i].booked) {
        JsonObject bookingObj = bookingsArray.createNestedObject();
        bookingObj["slot"] = slots[i].slotName;
        bookingObj["userName"] = slots[i].bookedBy;
        bookingObj["vehicle"] = slots[i].vehicleNumber;
        bookingObj["time"] = slots[i].bookingTime;
        bookingObj["status"] = "Active";
      }
    }
    
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
  }

  void handleGateOpen() {
    controlGate(true);
    server.send(200, "application/json", "{\"success\":true,\"message\":\"Gate opened instantly\"}");
  }

  void handleGateClose() {
    controlGate(false);
    server.send(200, "application/json", "{\"success\":true,\"message\":\"Gate closed instantly\"}");
  }

  void updateDisplay() {
    lcd.clear();
    
    switch (displayMode) {
      case 0:
        lcd.print("Smart Parking");
        lcd.setCursor(0, 1);
        lcd.print("Free:" + String(availableSlots));
        break;
      case 1:
        lcd.print("Last User:");
        lcd.setCursor(0, 1);
        lcd.print(lastUser.substring(0, 16));
        break;
      case 2:
        lcd.print("Entries/Exits");
        lcd.setCursor(0, 1);
        lcd.print(String(totalEntries) + "/" + String(totalExits));
        break;
    }
    
    displayMode = (displayMode + 1) % 3;
  }
