# Full System Deployment Guide

## 1. Required hardware

- NodeMCU ESP8266, PIR HC-SR501, MC-38, red LED, relay module.
- AI Thinker ESP32-CAM and a stable 5V power supply.
- USB-to-TTL for flashing ESP32-CAM.
- A Windows computer running the Flask server.
- No microSD card is required.

## 2. ESP8266 wiring

| Component | ESP8266 |
| --- | --- |
| PIR OUT | D5 |
| MC-38 signal | D6 |
| Red LED | D7 |
| Relay signal | D1 |

The relay firmware is configured as active-low. Change `RELAY_ON` and
`RELAY_OFF` if the relay module uses active-high logic.

## 3. Start Flask

Run:

```powershell
powershell -ExecutionPolicy Bypass -File server/run_server.ps1
```

Open:

```text
http://192.168.0.101:5000
```

Allow inbound TCP port `5000` through Windows Firewall when prompted.

## 4. Flash ESP32-CAM

Open `esp32cam/esp32cam.ino` and confirm:

```cpp
const char* uploadUrl = "http://192.168.0.101:5000/upload";
const char* heartbeatUrl = "http://192.168.0.101:5000/camera/heartbeat";
```

Flash using board `AI Thinker ESP32-CAM`. Remove the `GPIO0-GND` connection
after flashing and reset the board. Serial Monitor prints the camera IP.

Test:

```text
http://<ESP32-CAM-IP>/capture?event=motion
http://<ESP32-CAM-IP>/latest
```

## 5. Flash ESP8266

Open `test1_copy_20260604165126/test1_copy_20260604165126.ino`. Replace
`cameraCaptureUrl` with the IP printed by ESP32-CAM, then flash board
`NodeMCU 1.0 (ESP-12E Module)`.

The ESP32-CAM currently detected on this network uses `192.168.0.103`.

## 6. Telegram commands

Send these messages to the bot:

```text
/arm
/disarm
/status
```

When an alarm occurs, Telegram receives the alert first, followed by the Flask
image URL after upload completes.

## 7. Flutter app

Install Flutter SDK, then run:

```powershell
powershell -ExecutionPolicy Bypass -File mobile/flutter_app/bootstrap.ps1
cd mobile/flutter_app
flutter run
```

The app provides ARM/DISARM, status, image gallery, and event history.

## 8. Internet-accessible image links

By default, Telegram image links use the local Flask address and work while the
phone is connected to the same network. For public links, expose Flask through
a secure reverse proxy and set before starting Flask:

```powershell
$env:PUBLIC_BASE_URL = "https://your-public-server.example"
```

## 9. Final demo

1. Start Flask, ESP32-CAM, and ESP8266.
2. Confirm camera status is `Online`.
3. ARM from web, Flutter, or Telegram.
4. Open the door or trigger PIR.
5. Confirm relay ON, LED blinking, Telegram alert, uploaded photo, and SQLite
   history entry.
6. DISARM and confirm relay/LED turn off.
