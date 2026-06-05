#include "esp_camera.h"
#include <HTTPClient.h>
#include <WebServer.h>
#include <WiFi.h>

#if __has_include("wifi_config.h")
#include "wifi_config.h"
#else
const char* ssid = "YOUR_WIFI";
const char* password = "YOUR_PASSWORD";
const char* uploadUrl = "http://YOUR_SERVER_IP:5000/upload";
const char* heartbeatUrl = "http://YOUR_SERVER_IP:5000/camera/heartbeat";
#endif

// AI Thinker ESP32-CAM pin mapping
#define PWDN_GPIO_NUM  32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM   0
#define SIOD_GPIO_NUM  26
#define SIOC_GPIO_NUM  27
#define Y9_GPIO_NUM    35
#define Y8_GPIO_NUM    34
#define Y7_GPIO_NUM    39
#define Y6_GPIO_NUM    36
#define Y5_GPIO_NUM    21
#define Y4_GPIO_NUM    19
#define Y3_GPIO_NUM    18
#define Y2_GPIO_NUM     5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM  23
#define PCLK_GPIO_NUM  22

WebServer server(80);
uint8_t* latestImage = nullptr;
size_t latestImageLength = 0;
unsigned long latestCaptureNumber = 0;

bool initializeCamera()
{
  camera_config_t config = {};
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.grab_mode = CAMERA_GRAB_LATEST;
  config.fb_location = CAMERA_FB_IN_PSRAM;

  if (psramFound())
  {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  }
  else
  {
    config.frame_size = FRAMESIZE_VGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
    config.fb_location = CAMERA_FB_IN_DRAM;
  }

  esp_err_t error = esp_camera_init(&config);
  if (error != ESP_OK)
  {
    Serial.printf("Camera initialization failed: 0x%x\n", error);
    return false;
  }

  return true;
}

void connectWiFi()
{
  WiFi.mode(WIFI_STA);
  WiFi.setHostname("esp32cam");
  WiFi.begin(ssid, password);

  Serial.print("Connecting WiFi");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("ESP32-CAM IP: ");
  Serial.println(WiFi.localIP());
}

bool saveFrameToRam(camera_fb_t* frame)
{
  uint8_t* newImage = psramFound()
    ? static_cast<uint8_t*>(ps_malloc(frame->len))
    : static_cast<uint8_t*>(malloc(frame->len));
  if (!newImage)
  {
    Serial.println("Cannot allocate RAM for JPEG");
    return false;
  }

  memcpy(newImage, frame->buf, frame->len);
  if (latestImage)
  {
    free(latestImage);
  }

  latestImage = newImage;
  latestImageLength = frame->len;
  latestCaptureNumber++;
  Serial.printf("JPEG stored in RAM: %u bytes\n", latestImageLength);
  return true;
}

String uploadFrame(camera_fb_t* frame, const String& eventType)
{
  WiFiClient client;
  HTTPClient http;

  if (!http.begin(client, uploadUrl))
  {
    Serial.println("Upload failed: invalid server URL");
    return "";
  }

  http.addHeader("Content-Type", "image/jpeg");
  http.addHeader("X-Event-Type", eventType);
  http.setTimeout(15000);

  int httpCode = http.POST(frame->buf, frame->len);
  String response = httpCode > 0 ? http.getString() : "";
  Serial.printf("Image upload HTTP code: %d\n", httpCode);
  if (response.length())
  {
    Serial.println(response);
  }
  http.end();

  return httpCode == 200 ? response : "";
}

void sendHeartbeat()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    return;
  }

  WiFiClient client;
  HTTPClient http;
  if (http.begin(client, heartbeatUrl))
  {
    http.setTimeout(3000);
    http.POST("");
    http.end();
  }
}

void handleStatus()
{
  String response = String("{") +
    "\"camera\":\"online\"," +
    "\"storage\":\"ram\"," +
    "\"image_available\":" + (latestImage ? "true" : "false") + "," +
    "\"latest_bytes\":" + String(latestImageLength) + "," +
    "\"capture_number\":" + String(latestCaptureNumber) + "," +
    "\"capture_url\":\"http://" + WiFi.localIP().toString() + "/capture\"," +
    "\"latest_url\":\"http://" + WiFi.localIP().toString() + "/latest\"" +
    "}";

  server.send(200, "application/json", response);
}

void handleCapture()
{
  String eventType = server.hasArg("event") ? server.arg("event") : "intrusion";
  camera_fb_t* frame = esp_camera_fb_get();
  if (!frame)
  {
    server.send(500, "application/json", "{\"success\":false,\"error\":\"Camera capture failed\"}");
    return;
  }

  bool saved = saveFrameToRam(frame);
  String uploadResponse = uploadFrame(frame, eventType);
  size_t imageSize = frame->len;
  esp_camera_fb_return(frame);

  if (!saved)
  {
    server.send(500, "application/json", "{\"success\":false,\"error\":\"JPEG was not stored in RAM\"}");
    return;
  }

  String response = uploadResponse.length() > 0
    ? uploadResponse
    : String("{") +
      "\"success\":true," +
      "\"uploaded\":false," +
      "\"storage\":\"ram\"," +
      "\"capture_number\":" + String(latestCaptureNumber) + "," +
      "\"bytes\":" + String(imageSize) + "," +
      "\"latest_url\":\"http://" + WiFi.localIP().toString() + "/latest\"" +
      "}";

  server.send(200, "application/json", response);
}

void handleLatest()
{
  if (!latestImage || latestImageLength == 0)
  {
    server.send(404, "application/json", "{\"error\":\"No captured image available\"}");
    return;
  }

  server.sendHeader("Cache-Control", "no-store");
  server.setContentLength(latestImageLength);
  server.send(200, "image/jpeg", "");
  server.sendContent(reinterpret_cast<const char*>(latestImage), latestImageLength);
}

void setup()
{
  Serial.begin(115200);
  Serial.setDebugOutput(false);

  if (!initializeCamera())
  {
    delay(5000);
    ESP.restart();
  }

  connectWiFi();

  server.on("/", HTTP_GET, handleStatus);
  server.on("/capture", HTTP_GET, handleCapture);
  server.on("/latest", HTTP_GET, handleLatest);
  server.onNotFound([]()
  {
    server.send(404, "application/json", "{\"error\":\"Not found\"}");
  });
  server.begin();

  Serial.println("ESP32-CAM API ready");
  Serial.print("Capture API: http://");
  Serial.print(WiFi.localIP());
  Serial.println("/capture");
}

void loop()
{
  static unsigned long lastHeartbeat = 0;

  if (WiFi.status() != WL_CONNECTED)
  {
    connectWiFi();
  }

  if (millis() - lastHeartbeat > 30000)
  {
    sendHeartbeat();
    lastHeartbeat = millis();
  }

  server.handleClient();
  delay(2);
}
