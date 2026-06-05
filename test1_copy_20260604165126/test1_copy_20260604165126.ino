#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>

#if __has_include("secrets_config.h")
#include "secrets_config.h"
#else
const char* ssid = "YOUR_WIFI";
const char* password = "YOUR_PASSWORD";
const char* serverUrl = "http://YOUR_SERVER_IP:5000/status";
const char* updateUrl = "http://YOUR_SERVER_IP:5000/update_status";
const char* cameraCaptureUrl = "http://YOUR_ESP32_CAM_IP/capture";
const char* telegramBotToken = "YOUR_TELEGRAM_BOT_TOKEN";
const char* telegramChatId = "YOUR_TELEGRAM_CHAT_ID";
#endif

#define PIR_PIN   D5
#define DOOR_PIN  D6
#define LED_PIN   D7
#define RELAY_PIN D1

const int RELAY_ON = LOW;
const int RELAY_OFF = HIGH;

bool systemArmed = false;
bool lastDoorState = false;
bool lastMotion = false;
bool alarmActive = false;
long lastTelegramUpdateId = 0;
String currentDoorStatus = "Door Closed";
String currentMotionStatus = "No Motion";

// =========================
// CONNECT WIFI
// =========================
void connectWiFi()
{
  WiFi.mode(WIFI_STA);
  WiFi.persistent(false);
  WiFi.disconnect();
  delay(200);

  Serial.print("Connecting WiFi to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  unsigned long startedAt = millis();
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");

    if (millis() - startedAt > 20000)
    {
      Serial.println();
      Serial.print("WiFi failed, status code: ");
      Serial.println(WiFi.status());
      Serial.println("ESP8266 only supports 2.4 GHz WiFi. Check SSID, password, and hotspot/router band.");
      startedAt = millis();
      WiFi.disconnect();
      delay(500);
      WiFi.begin(ssid, password);
    }
  }

  Serial.println();
  Serial.println("WiFi Connected");

  Serial.print("ESP IP: ");
  Serial.println(WiFi.localIP());
}

// =========================
// READ STATUS FROM SERVER
// =========================
void updateSystemStatus()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("WiFi Lost - Reconnecting");
    connectWiFi();
    return;
  }

  WiFiClient client;
  HTTPClient http;

  http.begin(client, serverUrl);

  int httpCode = http.GET();

  if (httpCode == 200)
  {
    String payload = http.getString();

    Serial.println("========== SERVER ==========");
    Serial.println(payload);

    if (payload.indexOf("DISARMED") >= 0)
    {
      systemArmed = false;
      alarmActive = false;
      digitalWrite(RELAY_PIN, RELAY_OFF);
      digitalWrite(LED_PIN, LOW);
    }
    else if (payload.indexOf("ARMED") >= 0)
    {
      systemArmed = true;
    }

    Serial.print("System Status: ");

    if(systemArmed)
      Serial.println("ARMED");
    else
      Serial.println("DISARMED");
  }
  else
  {
    Serial.print("HTTP Error: ");
    Serial.println(httpCode);
  }

  http.end();
}

void sendStatusToServer(String motionStatus, String doorStatus, String eventType, String description, bool logEvent = true) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Cannot POST: WiFi not connected");
    return;
  }

  WiFiClient client;
  HTTPClient http;

  String json = String("{") +
    "\"motion_status\":\"" + motionStatus + "\"," +
    "\"door_status\":\"" + doorStatus + "\"," +
    "\"event_type\":\"" + eventType + "\"," +
    "\"description\":\"" + description + "\"," +
    "\"log_event\":" + (logEvent ? "true" : "false") + "}";

  Serial.print("Posting to: ");
  Serial.println(updateUrl);
  Serial.print("Payload: ");
  Serial.println(json);

  const int maxRetries = 3;
  int attempt = 0;
  int code = -1;
  while (attempt < maxRetries) {
    attempt++;
    Serial.print("Attempt "); Serial.print(attempt); Serial.println("...");
    http.begin(client, updateUrl);
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(10000);
    code = http.POST(json);
    Serial.print("POST returned: "); Serial.println(code);
    http.end();
    if (code > 0) break;
    delay(500);
  }

  if (code <= 0) {
    Serial.println("POST failed after retries. Check server IP, port, and firewall.");
  }
}

// =========================
// TRIGGER ESP32-CAM
// =========================
String extractJsonString(const String& json, const String& key)
{
  String marker = "\"" + key + "\":\"";
  int start = json.indexOf(marker);
  if (start < 0)
  {
    return "";
  }

  start += marker.length();
  int end = json.indexOf('"', start);
  return end >= 0 ? json.substring(start, end) : "";
}

String triggerCameraCapture(const String& eventType)
{
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("Camera trigger failed: WiFi not connected");
    return "";
  }

  WiFiClient client;
  HTTPClient http;
  String captureUrl = String(cameraCaptureUrl) + "?event=" + eventType;

  if (!http.begin(client, captureUrl))
  {
    Serial.println("Camera trigger failed: invalid URL");
    return "";
  }

  http.setTimeout(20000);
  int httpCode = http.GET();

  if (httpCode == 200)
  {
    String payload = http.getString();
    Serial.println("ESP32-CAM captured a JPEG");
    Serial.println(payload);
    http.end();
    return extractJsonString(payload, "image_url");
  }

  Serial.print("ESP32-CAM HTTP error: ");
  Serial.println(httpCode);
  http.end();
  return "";
}

String urlEncode(const String& value)
{
  String encoded = "";
  char hex[] = "0123456789ABCDEF";

  for (unsigned int i = 0; i < value.length(); i++)
  {
    unsigned char c = value.charAt(i);

    if ((c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9') ||
        c == '-' || c == '_' || c == '.' || c == '~')
    {
      encoded += (char)c;
    }
    else
    {
      encoded += '%';
      encoded += hex[c >> 4];
      encoded += hex[c & 0x0F];
    }
  }

  return encoded;
}

// =========================
// SEND TELEGRAM ALERT
// =========================
bool sendTelegramAlert(const String& message)
{
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("Telegram failed: WiFi not connected");
    return false;
  }

  if (String(telegramBotToken) == "YOUR_BOT_TOKEN" ||
      String(telegramChatId) == "YOUR_CHAT_ID")
  {
    Serial.println("Telegram skipped: configure BOT_TOKEN and CHAT_ID");
    return false;
  }

  BearSSL::WiFiClientSecure client;
  client.setInsecure();

  HTTPClient https;
  String url = "https://api.telegram.org/bot" + String(telegramBotToken) + "/sendMessage";

  if (!https.begin(client, url))
  {
    Serial.println("Telegram failed: cannot start HTTPS request");
    return false;
  }

  https.addHeader("Content-Type", "application/x-www-form-urlencoded");
  https.setTimeout(10000);

  String body = "chat_id=" + urlEncode(telegramChatId) +
                "&text=" + urlEncode(message);
  int httpCode = https.POST(body);

  if (httpCode == 200)
  {
    Serial.println("Telegram alert sent");
    https.end();
    return true;
  }

  Serial.print("Telegram HTTP error: ");
  Serial.println(httpCode);
  if (httpCode > 0)
  {
    Serial.println(https.getString());
  }

  https.end();
  return false;
}

bool setServerArmState(bool armed)
{
  if (WiFi.status() != WL_CONNECTED)
  {
    return false;
  }

  WiFiClient client;
  HTTPClient http;
  String url = String("http://10.244.81.38:5000/") + (armed ? "arm" : "disarm");
  if (!http.begin(client, url))
  {
    return false;
  }

  http.setTimeout(5000);
  int code = http.POST("");
  http.end();
  return code == 200;
}

void processTelegramCommand(const String& command)
{
  if (command == "/arm")
  {
    systemArmed = true;
    setServerArmState(true);
    sendTelegramAlert("System Armed");
  }
  else if (command == "/disarm")
  {
    systemArmed = false;
    alarmActive = false;
    digitalWrite(RELAY_PIN, RELAY_OFF);
    digitalWrite(LED_PIN, LOW);
    setServerArmState(false);
    sendTelegramAlert("System Disarmed");
  }
  else if (command == "/status")
  {
    String status = systemArmed ? "ARMED" : "DISARMED";
    sendTelegramAlert(
      "System: " + status +
      "\nPIR: " + currentMotionStatus +
      "\nDoor: " + currentDoorStatus
    );
  }
}

void checkTelegramCommands()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    return;
  }

  BearSSL::WiFiClientSecure client;
  client.setInsecure();
  HTTPClient https;
  String url = "https://api.telegram.org/bot" + String(telegramBotToken) +
               "/getUpdates?offset=" + String(lastTelegramUpdateId + 1) + "&timeout=0";
  if (!https.begin(client, url))
  {
    return;
  }

  https.setTimeout(5000);
  int code = https.GET();
  if (code != 200)
  {
    https.end();
    return;
  }

  String payload = https.getString();
  https.end();

  int updatePosition = payload.lastIndexOf("\"update_id\":");
  if (updatePosition < 0)
  {
    return;
  }

  int idStart = updatePosition + 12;
  int idEnd = payload.indexOf(',', idStart);
  lastTelegramUpdateId = payload.substring(idStart, idEnd).toInt();

  int textStart = payload.indexOf("\"text\":\"", updatePosition);
  if (textStart < 0)
  {
    return;
  }

  int chatStart = payload.indexOf("\"chat\":{\"id\":", updatePosition);
  if (chatStart < 0)
  {
    return;
  }
  chatStart += 13;
  int chatEnd = payload.indexOf(',', chatStart);
  String commandChatId = payload.substring(chatStart, chatEnd);
  if (commandChatId != telegramChatId)
  {
    Serial.println("Ignored Telegram command from unauthorized chat");
    return;
  }

  textStart += 8;
  int textEnd = payload.indexOf('"', textStart);
  if (textEnd >= 0)
  {
    processTelegramCommand(payload.substring(textStart, textEnd));
  }
}

void activateAlarm()
{
  alarmActive = true;
  digitalWrite(RELAY_PIN, RELAY_ON);
}

void updateAlarmOutputs()
{
  static unsigned long lastBlink = 0;
  static bool ledState = false;

  if (!alarmActive)
  {
    digitalWrite(LED_PIN, LOW);
    return;
  }

  if (millis() - lastBlink >= 300)
  {
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState ? HIGH : LOW);
    lastBlink = millis();
  }
}

// =========================
// SETUP
// =========================
void setup()
{
  Serial.begin(115200);

  pinMode(PIR_PIN, INPUT);

  pinMode(DOOR_PIN, INPUT_PULLUP);

  pinMode(LED_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);

  digitalWrite(LED_PIN, LOW);
  digitalWrite(RELAY_PIN, RELAY_OFF);

  connectWiFi();

  lastDoorState = digitalRead(DOOR_PIN);
  currentDoorStatus = lastDoorState ? "Door Open" : "Door Closed";

  Serial.println();
  Serial.println("================================");
  Serial.println("SMART SECURITY SYSTEM READY");
  Serial.println("================================");
}

// =========================
// LOOP
// =========================
void loop()
{
  // Cập nhật trạng thái ARM/DISARM mỗi 2 giây
  static unsigned long lastUpdate = 0;
  static unsigned long lastTelegramPoll = 0;

  if (millis() - lastUpdate > 2000)
  {
    updateSystemStatus();
    lastUpdate = millis();
  }

  if (millis() - lastTelegramPoll > 3000)
  {
    checkTelegramCommands();
    lastTelegramPoll = millis();
  }

  updateAlarmOutputs();

  // =====================
  // READ SENSORS
  // =====================
  bool doorState = digitalRead(DOOR_PIN);
  bool motion = digitalRead(PIR_PIN);

  // Door: send only on change
  if (doorState != lastDoorState)
  {
    currentDoorStatus = doorState ? "Door Open" : "Door Closed";
    if (doorState == HIGH)
    {
      Serial.println("[INFO] Door Open");
      if (systemArmed)
      {
        activateAlarm();
        sendTelegramAlert("\xE2\x9A\xA0 Door Opened");
        String imageUrl = triggerCameraCapture("door_open");
        if (imageUrl.length() > 0)
        {
          sendTelegramAlert("Photo: " + imageUrl);
        }
        sendStatusToServer(
          currentMotionStatus,
          currentDoorStatus,
          "door_open",
          "Door Open",
          imageUrl.length() == 0
        );
      }
    }
    else
    {
      Serial.println("[INFO] Door Close");
      if (systemArmed)
      {
        sendStatusToServer(currentMotionStatus, currentDoorStatus, "door_close", "Door Close");
      }
    }

    lastDoorState = doorState;
  }

  // Motion: send only on change and when system is armed
  if (motion != lastMotion)
  {
    currentMotionStatus = motion ? "Motion Detected" : "No Motion";
    if (motion)
    {
      Serial.println("[ALERT] Motion Detected");
      if (systemArmed)
      {
        activateAlarm();
        sendTelegramAlert("\xE2\x9A\xA0 Motion Detected");
        String imageUrl = triggerCameraCapture("motion");
        if (imageUrl.length() > 0)
        {
          sendTelegramAlert("Photo: " + imageUrl);
        }
        sendStatusToServer(
          currentMotionStatus,
          currentDoorStatus,
          "motion",
          "Motion Detected",
          imageUrl.length() == 0
        );
      }
    }
    else
    {
      Serial.println("[INFO] Motion Ended");
      if (systemArmed)
      {
        sendStatusToServer(currentMotionStatus, currentDoorStatus, "motion_end", "Motion Ended");
      }
    }

    lastMotion = motion;
  }

  delay(100);
}
