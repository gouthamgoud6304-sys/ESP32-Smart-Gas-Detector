// --- LIBRARIES ---
#include <WiFi.h>
#include <WebServer.h>      
#include <SPIFFS.h>           
#include <WiFiManager.h>      
#include <ArduinoJson.h>      
#include <ArduinoOTA.h>       
#include <HTTPClient.h>      
#include "time.h"             
#include <Wire.h> 
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFiClientSecure.h> 
#include <ESP_Mail_Client.h>  

// --- FIRMWARE VERSION ---
const String FIRMWARE_VERSION = "GD-080126-V3"; 

// WIFI & CAPTIVE PORTAL CONFIG
const char* CAPTIVE_PORTAL_SSID = "ESP32-GD-SETUP";

const String WA_API_KEY = "YOUR WHATSAPP API KEY"; 

// RTC MEMORY
RTC_DATA_ATTR int bootCount = 0; 

// --- Email Config (Gmail SMTP) ---
#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT 467
#define SENDER_EMAIL "ENTER YOUR GMAIL ID"
#define SENDER_PASSWORD "ENTER YOUR GMAIL APP PASSWORD" 

SMTPSession smtp; 
const String BOT_TOKEN = "YOUR_TELEGRAM_BOT_TOKEN";
const String CHAT_ID = "YOUR BOT CHAT ID";

// --- PROFESSIONAL BRANDED CSS & SUCCESS LOGIC ---
const char* custom_html_head = R"html(
<style>
  body { background-color: #ffffffff; color: #1a1a1b; font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; text-align: center; margin: 0; padding: 20px; }
  .logo-container { margin-bottom: 15px; }
  .logo-container img { max-width: 150px; height: auto; display: block; margin: 0 auto; }
  .c { background-color: #ffffff; border-radius: 15px; padding: 30px; box-shadow: 0 10px 25px rgba(0,0,0,0.1); margin: 20px auto; max-width: 420px; border-top: 5px solid #fc9e9eff; }
  h1 { color: #f86f6fff; text-transform: uppercase; letter-spacing: 1px; font-size: 1.5em; margin-bottom: 25px; font-weight: 800; }
  input { background: #97def4ff; color: #333; border: 1px solid #f9855bff; padding: 12px; border-radius: 8px; margin: 10px 0; width: 100%; box-sizing: border-box; transition: 0.3s; }
  input:focus { border-color: #fa8b8bff; outline: none; box-shadow: 0 0 5px rgba(211,47,47,0.2); }
  button { background-color: #97def4ff; color: white; border: none; padding: 15px; border-radius: 8px; font-weight: bold; cursor: pointer; text-transform: uppercase; width: 100%; margin-top: 15px; font-size: 1em; transition: 0.3s; }
  button:hover { background-color: #ff8f8fff; }
  label { display: block; text-align: left; font-size: 0.85em; color: #0c0c0cff; margin-top: 10px; font-weight: 600; }
  .success-msg { background: #61f36dff; color: #00ea0cff; padding: 25px; border-radius: 12px; border: 1px solid #27ce7eff; }
</style>
<script>
  function insertLogo() {
    var container = document.querySelector('.c') || document.querySelector('div');
    if (container && !document.querySelector('.logo-container')) {
      var logoDiv = document.createElement('div');
      logoDiv.className = 'logo-container';
      var img = document.createElement('img');
      img.src = "data:image/webp;base64,"ADD YOU LOGO DATA BITS"; 
      
      logoDiv.appendChild(img);
      container.insertBefore(logoDiv, container.firstChild);
    }
  }
  
  window.onload = function() {
    insertLogo();
    setTimeout(insertLogo, 500); 
    
    var h1 = document.querySelector('h1');
    if (h1) h1.innerHTML = 'SMART GAS DETECTOR';
    if(window.location.hash == '#success') {
        document.body.innerHTML = '<div class="c success-msg"><h1>SYSTEM ONLINE</h1><p>Settings saved successfully.</p></div>';
    }
  };
</script>
)html";

// --- DYNAMIC USER GLOBALS ---
char user_mobile[16] = "ENTER YOUR MOBILE NUMBER"; 
char user_email[40] = "ENETR USER MAIL ID";
char device_location[20] = "Kitchen";
char gas_threshold_char[6] = "1500"; 
char ota_password[16] = "ota_admin"; 

// --- STORAGE PATHS ---
const char* configFilePath = "/config.json"; 
const char* backupPath = "/wifi_backup.json";

// --- STABILITY & MULTITASKING GLOBALS ---
bool alertsSent = false; 
unsigned long lastAlertTime = 0;
const unsigned long ALERT_REMINDER_INTERVAL = 300000; 

TaskHandle_t AlertTaskHandle = NULL;
TaskHandle_t SensorTaskHandle = NULL;
volatile int backgroundGasValue = 0;
volatile bool triggerBackgroundAlert = false;
volatile bool buzzerMuted = false; 
volatile bool isPreheating = true; 
volatile bool tasksPaused = false; 
const int PREHEAT_TIME_SECONDS = 10; 

unsigned long lastReconnectAttempt = 2;
const unsigned long RECONNECT_INTERVAL = 30000;
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

// --- HARDWARE PINS ---
#define MQ_SENSOR_PIN           34       
#define BUZZER_PIN               26       
#define LED_ALERT_PIN           27       
#define CONFIG_BUTTON_PIN       32       
#define BUZZER_STOP_BUTTON_PIN  25       

// --- OLED CONFIGURATION ---
const int SCREEN_WIDTH = 128;
const int SCREEN_HEIGHT = 64;
#define OLED_SDA_PIN 21
#define OLED_SCL_PIN 22
#define OLED_I2C_ADDRESS 0x3C

// --- ALARM SETTINGS ---
const int ALARM_RESET_THRESHOLD = 980; 
volatile bool isBuzzerOn = false;     
bool isAlarmActive = false; 
const unsigned long BLINK_INTERVAL = 250; 

// --- BEEP PULSE SETTINGS ---
unsigned long lastBeepMillis = 0;
bool beepState = false;
const int BEEP_INTERVAL = 200; 

// --- TIME CONFIGURATION ---
const long gmtOffset_sec = 19800; 
const char* time_zone_config = "IST-5:30";
const char* NTP_SERVER_1 = "pool.ntp.org"; 
const char* NTP_SERVER_2 = "time.nist.gov";
const unsigned long TIME_UPDATE_INTERVAL = 20000; 
const unsigned long RESET_HOLD_TIME_MS = 3000; 

// --- GLOBAL OBJECTS ---
bool shouldSaveConfig = false;
unsigned long previousMillis = 0;
unsigned long lastTimeUpdateMillis = 0; 
const long interval = 2000; 
unsigned long configButtonPressStartTime = 0; 
bool time_is_synced = false; 
unsigned long lastDisplayUpdate = 0;
const unsigned long DISPLAY_UPDATE_INTERVAL = 1000; 

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1); 

// --- FUNCTION PROTOTYPES ---
void saveConfigCallback ();
void IRAM_ATTR buzzerStopISR();
void checkConfigButtonHold(); 
void readAndSendSensorData();
void setupOTA();
void setupNTP(); 
void updateNTP(); 
String getCurrentDateTime();
String getCurrentDate();
void initializeOLED(); 
void displayStatus(int gasValue, int threshold); 
void displayAlertScreen(int gasValue, int threshold); 
void drawWiFiIcon();
void sendSmartAlerts(int gasValue);
void backgroundAlertTask(void * pvParameters); 
void sensorProcessingTask(void * pvParameters);
void sensorPreheatRoutine();
void backupCurrentWiFi();
void rollbackToOldWiFi();
bool saveConfig();
bool loadConfig();

// --- FILE SYSTEM FUNCTIONS ---
bool loadConfig() {
  if (!SPIFFS.begin(true)) return false;
  File configFile = SPIFFS.open(configFilePath, "r");
  if (!configFile) { SPIFFS.end(); return false; }
  
  JsonDocument doc; 
  DeserializationError error = deserializeJson(doc, configFile); 
  configFile.close();
  if (error) { SPIFFS.end(); return false; }
  
  strlcpy(ota_password, doc["ota_password"] | "ota_admin", sizeof(ota_password));
  strlcpy(gas_threshold_char, doc["gas_threshold"] | "1600", sizeof(gas_threshold_char)); 
  strlcpy(user_mobile, doc["user_mobile"] | "91XXXXXXXXXX", sizeof(user_mobile));
  strlcpy(user_email, doc["user_email"] | "USER MAIL ID", sizeof(user_email));
  strlcpy(device_location, doc["device_location"] | "Kitchen", sizeof(device_location));

  SPIFFS.end(); return true;
}

bool saveConfig() {
  if (!SPIFFS.begin(true)) return false;
  JsonDocument doc;
  doc["ota_password"] = ota_password; 
  doc["gas_threshold"] = gas_threshold_char;
  doc["user_mobile"] = user_mobile;
  doc["user_email"] = user_email;
  doc["device_location"] = device_location;
  File configFile = SPIFFS.open(configFilePath, "w");
  if (!configFile) { SPIFFS.end(); return false; }
  serializeJson(doc, configFile);
  configFile.close(); SPIFFS.end(); return true;
}

// --- WIFI ROLLBACK LOGIC ---
void backupCurrentWiFi() {
  if (!SPIFFS.begin(true)) return;
  JsonDocument doc;
  doc["ssid"] = WiFi.SSID();
  doc["pass"] = WiFi.psk();
  File f = SPIFFS.open(backupPath, "w");
  if (f) { serializeJson(doc, f); f.close(); }
  SPIFFS.end();
}

void rollbackToOldWiFi() {
  if (!SPIFFS.begin(true)) return;
  File f = SPIFFS.open(backupPath, "r");
  if (f) {
    JsonDocument doc;
    deserializeJson(doc, f);
    String oldSSID = doc["ssid"];
    String oldPASS = doc["pass"];
    f.close();
    Serial.printf("[ROLLBACK] Reconnecting to: %s\n", oldSSID.c_str());
    WiFi.begin(oldSSID.c_str(), oldPASS.c_str());
  }
  SPIFFS.end();
}

void saveConfigCallback () { shouldSaveConfig = true; }

// --- ENHANCED ACTIONABLE ALERTS WITH REGISTERED INFO ---
void sendSmartAlerts(int gasValue) {
  if (WiFi.status() == WL_CONNECTED) {
    String currentTime = getCurrentDateTime();
    String currentDate = getCurrentDate();
    String loc = String(device_location);
    String recipientMail = String(user_email);
    String recipientMobile = String(user_mobile);

    // 1. WhatsApp & Telegram Message
    String rawText = "🚨 EMERGENCY: Gas Leak at " + loc + "!\n";
    rawText += "Gas Level: " + String(gasValue) + "\n";
    rawText += "Time: " + currentTime + "\n";
    rawText += "Reg Mobile: " + recipientMobile + "\n";
    rawText += "Reg Email: " + recipientMail;

    String encodedText = rawText; encodedText.replace("\n", "%0A"); encodedText.replace(" ", "%20");
    
    HTTPClient http;
    http.begin("http://api.callmebot.com/whatsapp.php?phone=" + recipientMobile + "&text=" + encodedText + "&apikey=" + WA_API_KEY);
    http.GET(); http.end();

    WiFiClientSecure clientTG; clientTG.setInsecure();
    http.begin(clientTG, "https://api.telegram.org/bot" + BOT_TOKEN + "/sendMessage?chat_id=" + CHAT_ID + "&text=" + encodedText);
    http.GET(); http.end();

    // 2. Actionable HTML Email
    ESP_Mail_Session session; session.server.host_name = SMTP_HOST; session.server.port = SMTP_PORT;
    session.login.email = SENDER_EMAIL; session.login.password = SENDER_PASSWORD;
    
    SMTP_Message message;
    message.sender.name = "Smart Safety System"; 
    message.sender.email = SENDER_EMAIL;
    message.subject = "CRITICAL: Gas Leak Detected - " + loc;
    message.addRecipient("Homeowner", recipientMail.c_str());

    String htmlMsg = "<div style='font-family:sans-serif; border:3px solid #d32f2f; padding:20px; border-radius:15px; max-width:500px;'>";
    htmlMsg += "<h1 style='color:#d32f2f; margin-top:0;'>⚠️ GAS LEAK ALERT</h1>";
    htmlMsg += "<p style='font-size:1.1em;'>A gas leak has been detected at <b>" + loc + "</b>.</p>";
    htmlMsg += "<div style='background:#fdeaea; padding:15px; border-radius:10px; border-left:5px solid #d32f2f;'>";
    htmlMsg += "<b>Concentration:</b> " + String(gasValue) + "<br>";
    htmlMsg += "<b>Reg Mobile:</b> " + recipientMobile + "<br>";
    htmlMsg += "<b>Reg Email:</b> " + recipientMail + "<br>";
    htmlMsg += "<b>Date/Time:</b> " + currentDate + " | " + currentTime + "</div>";
    htmlMsg += "<h3>🚨 EMERGENCY STEPS:</h3><ol>";
    htmlMsg += "<li><b>Evacuate</b> the area immediately.</li>";
    htmlMsg += "<li>Do <b>NOT</b> switch on lights or appliances.</li>";
    htmlMsg += "<li>Open all windows and doors for ventilation.</li>";
    htmlMsg += "<li>Call emergency services if necessary.</li></ol>";
    htmlMsg += "</div>";

    message.html.content = htmlMsg.c_str();
    if (smtp.connect(&session)) {
      MailClient.sendMail(&smtp, &message);
      smtp.closeSession();
    }
  }
}

// --- FREERTOS TASKS ---
void backgroundAlertTask(void * pvParameters) {
  for(;;) {
    if (!tasksPaused && triggerBackgroundAlert && !isPreheating) {
      sendSmartAlerts(backgroundGasValue);
      portENTER_CRITICAL(&mux); triggerBackgroundAlert = false; portEXIT_CRITICAL(&mux);
    }
    vTaskDelay(pdMS_TO_TICKS(500)); 
  }
}

void sensorProcessingTask(void * pvParameters) {
  for(;;) {
    if (!tasksPaused && !isPreheating) { readAndSendSensorData(); }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void sensorPreheatRoutine() {
  for (int i = PREHEAT_TIME_SECONDS; i > 0; i--) {
    display.clearDisplay(); display.setTextColor(SSD1306_WHITE); display.setTextSize(1);
    display.setCursor(0, 5); display.println("INITIALIZING SENSOR...");
    int barWidth = map(i, 0, PREHEAT_TIME_SECONDS, 128, 0);
    display.drawRect(0, 20, 128, 10, SSD1306_WHITE); display.fillRect(0, 20, barWidth, 10, SSD1306_WHITE);
    display.setCursor(45, 35); display.print(i); display.print("s");
    display.display(); delay(1000);
  }
  isPreheating = false;
}

void IRAM_ATTR buzzerStopISR() {
  digitalWrite(BUZZER_PIN, LOW); 
  isBuzzerOn = false; buzzerMuted = true; 
}

void checkConfigButtonHold() {
  if (digitalRead(CONFIG_BUTTON_PIN) == LOW) {
    if (configButtonPressStartTime == 0) configButtonPressStartTime = millis();
    else if (millis() - configButtonPressStartTime >= RESET_HOLD_TIME_MS) {
      
      detachInterrupt(digitalPinToInterrupt(BUZZER_STOP_BUTTON_PIN));
      tasksPaused = true; 
      if(AlertTaskHandle != NULL) vTaskSuspend(AlertTaskHandle);
      if(SensorTaskHandle != NULL) vTaskSuspend(SensorTaskHandle);
      
      display.clearDisplay(); 
      display.setCursor(0, 20); 
      display.println("WIFI PORTAL OPENED"); 
      display.setCursor(0, 35);
      display.println("Connect to:");
      display.println(CAPTIVE_PORTAL_SSID);
      display.display();
      
      WiFi.mode(WIFI_OFF);
      delay(100);
      
      WiFiManager wm;
      wm.setCustomHeadElement(custom_html_head);
      
      String macAddr = WiFi.macAddress();
      WiFiManagerParameter p_mac_id("mac_id", "Device MAC Address", macAddr.c_str(), 20, "readonly style='background:#eeeeee; color:#888;'");
      WiFiManagerParameter p_loc("loc", "Device Location", device_location, 20);
      WiFiManagerParameter p_user_mobile("User_Mob", "WhatsApp Mobile", user_mobile, 16);
      WiFiManagerParameter p_user_email("User_Email", "Alert Email", user_email, 40);
      WiFiManagerParameter p_ota_password("OTA_Pass", "Admin Password", ota_password, 16);
      WiFiManagerParameter p_gas_threshold("Gas_Thr", "Alert Threshold", gas_threshold_char, 6);

      wm.addParameter(&p_mac_id);
      wm.addParameter(&p_loc); 
      wm.addParameter(&p_user_mobile); 
      wm.addParameter(&p_user_email); 
      wm.addParameter(&p_ota_password); 
      wm.addParameter(&p_gas_threshold);
      
      wm.setConfigPortalTimeout(180);

      if (!wm.startConfigPortal(CAPTIVE_PORTAL_SSID, "password")) {
          Serial.println("Portal timeout, restarting...");
          ESP.restart(); 
      } else {
          strlcpy(device_location, p_loc.getValue(), sizeof(device_location));
          strlcpy(user_mobile, p_user_mobile.getValue(), sizeof(user_mobile));
          strlcpy(user_email, p_user_email.getValue(), sizeof(user_email));
          strlcpy(ota_password, p_ota_password.getValue(), sizeof(ota_password));
          strlcpy(gas_threshold_char, p_gas_threshold.getValue(), sizeof(gas_threshold_char));
          saveConfig(); 
          delay(1000); 
          ESP.restart(); 
      }
    }
  } else configButtonPressStartTime = 0;
}

void setupNTP() {
    configTime(gmtOffset_sec, 0, NTP_SERVER_1, NTP_SERVER_2);
    setenv("TZ", time_zone_config, 1); tzset(); 
    struct tm timeinfo; time_is_synced = getLocalTime(&timeinfo);
}

void updateNTP() {
    if (WiFi.status() == WL_CONNECTED && (millis() - lastTimeUpdateMillis >= TIME_UPDATE_INTERVAL)) {
        lastTimeUpdateMillis = millis();
        struct tm timeinfo; if(getLocalTime(&timeinfo)) time_is_synced = true;
    }
}

String getCurrentDateTime() {
  struct tm timeinfo;
  if (!time_is_synced || !getLocalTime(&timeinfo)) return "SYNCING...";
  char timeStr[10]; strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);
  return String(timeStr);
}

String getCurrentDate() {
  struct tm timeinfo;
  if (!time_is_synced || !getLocalTime(&timeinfo)) return "---";
  char dateStr[16]; strftime(dateStr, sizeof(dateStr), "%d-%m-%Y", &timeinfo);
  return String(dateStr);
}

void initializeOLED() {
    Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN); 
    if (display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDRESS)) {
        display.clearDisplay(); display.display();
    }
}

void setupOTA() {
  ArduinoOTA.setHostname("gas-detector-esp32");
  ArduinoOTA.begin();
}

void drawWiFiIcon() {
    if (WiFi.status() == WL_CONNECTED) {
        display.drawCircle(120, 5, 2, SSD1306_WHITE);
        display.drawCircle(120, 5, 4, SSD1306_WHITE);
    } else { 
        display.setCursor(80, 0); 
        display.setTextSize(1);
        display.print("     !"); 
    }
}

void readAndSendSensorData() {
  int gasThreshold = atoi(gas_threshold_char); 
  int gasValue = analogRead(MQ_SENSOR_PIN); 

  if (millis() - previousMillis >= interval) {
    previousMillis = millis();
    if (gasValue > gasThreshold) {
      isAlarmActive = true; digitalWrite(LED_ALERT_PIN, HIGH);
      if (!alertsSent || (millis() - lastAlertTime > ALERT_REMINDER_INTERVAL)) {
        portENTER_CRITICAL(&mux); 
        backgroundGasValue = gasValue; 
        triggerBackgroundAlert = true; 
        alertsSent = true; 
        lastAlertTime = millis();
        portEXIT_CRITICAL(&mux);
      }
    } else if (gasValue < ALARM_RESET_THRESHOLD) {
      isAlarmActive = false; digitalWrite(BUZZER_PIN, LOW); digitalWrite(LED_ALERT_PIN, LOW);
      isBuzzerOn = false; alertsSent = false; buzzerMuted = false;
    }
  }

  if (isAlarmActive && !buzzerMuted) {
    if (millis() - lastBeepMillis >= BEEP_INTERVAL) {
      lastBeepMillis = millis();
      beepState = !beepState;
      digitalWrite(BUZZER_PIN, beepState ? HIGH : LOW);
    }
  } else { digitalWrite(BUZZER_PIN, LOW); }

  displayStatus(gasValue, gasThreshold); 
}

void setup() {
  Serial.begin(115200);
  Serial.println("VERSION: GD-080126-V3");

  initializeOLED();
  pinMode(BUZZER_PIN, OUTPUT); pinMode(LED_ALERT_PIN, OUTPUT);
  pinMode(CONFIG_BUTTON_PIN, INPUT_PULLUP); pinMode(BUZZER_STOP_BUTTON_PIN, INPUT_PULLUP); 
  attachInterrupt(digitalPinToInterrupt(BUZZER_STOP_BUTTON_PIN), buzzerStopISR, FALLING); 

  loadConfig(); 
  
  esp_reset_reason_t reason = esp_reset_reason();
  if (reason == ESP_RST_POWERON || reason == ESP_RST_EXT) sensorPreheatRoutine();
  else isPreheating = false;

  xTaskCreatePinnedToCore(backgroundAlertTask, "AlertTask", 10000, NULL, 1, &AlertTaskHandle, 0);
  xTaskCreatePinnedToCore(sensorProcessingTask, "SensorTask", 10000, NULL, 1, &SensorTaskHandle, 1);

  WiFiManager wifiManager;
  wifiManager.setCustomHeadElement(custom_html_head);
  wifiManager.setConfigPortalTimeout(60); 

  if (!wifiManager.autoConnect(CAPTIVE_PORTAL_SSID, "password")) {
    rollbackToOldWiFi();
  } else {
    backupCurrentWiFi();
    setupNTP(); setupOTA();
  }
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) { ArduinoOTA.handle(); updateNTP(); }
  else if (millis() - lastReconnectAttempt >= RECONNECT_INTERVAL) {
      lastReconnectAttempt = millis(); 
      WiFi.begin(); 
  }
  checkConfigButtonHold(); 
  yield(); 
}

void displayAlertScreen(int gasValue, int threshold) {
    if (millis() - lastDisplayUpdate < BLINK_INTERVAL) return;
    lastDisplayUpdate = millis();
    display.clearDisplay();
    bool isBlinkOn = ((millis() / 500) % 2 == 0);
    display.setTextSize(2); display.setCursor(10, 5); 
    if (isBlinkOn) { display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); display.print(" DANGER "); }
    else { display.setTextColor(SSD1306_WHITE); display.print(" DANGER "); }
    display.setTextColor(SSD1306_WHITE); display.setTextSize(1); 
    display.setCursor(0, 35); display.print("GAS LEVEL: "); display.println(gasValue);
    display.setCursor(0, 48); display.print("LOC: "); display.println(device_location);
    display.display();
}

void displayStatus(int gasValue, int threshold) {
    if (isAlarmActive) { displayAlertScreen(gasValue, threshold); return; }
    if (millis() - lastDisplayUpdate < DISPLAY_UPDATE_INTERVAL) return;
    lastDisplayUpdate = millis();
    display.clearDisplay(); display.setTextColor(SSD1306_WHITE);
    
    drawWiFiIcon();
    
    display.setCursor(0, 0);
    display.setTextSize(2); 
    display.println(getCurrentDateTime());
    
    display.setTextSize(1);
    display.setCursor(0, 20);
    display.print("Date: "); display.println(getCurrentDate());
    
    display.setCursor(0, 32);
    display.print("Loc : "); display.println(device_location); 
    
    display.setCursor(0, 44);
    display.print("Gas : "); display.print(gasValue);
    display.print(" (Thr:"); display.print(threshold); display.println(")");
    
    display.setCursor(0, 56);
    if (gasValue > ALARM_RESET_THRESHOLD) display.println("STATUS: CLEANING...");
    else display.println("STATUS: SAFE");
    
    display.display();
}
