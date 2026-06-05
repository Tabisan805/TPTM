# Smart Home Security Architecture

```text
PIR + MC-38
     |
     v
ESP8266 -------- Telegram Bot
  | ARM/DISARM      | alerts + image link
  | alarm output
  | /capture?event=...
  v
ESP32-CAM
  | JPEG POST /upload
  v
Flask Server
  |-- SQLite event history
  |-- static/uploads JPEG files
  |-- Web dashboard
  `-- REST API for Flutter
```

## Main API

| Method | Endpoint | Purpose |
| --- | --- | --- |
| GET | `/status` | Current system status |
| POST | `/arm` | Arm system |
| POST | `/disarm` | Disarm system |
| POST | `/update_status` | ESP8266 sensor update |
| POST | `/upload` | ESP32-CAM JPEG upload |
| POST | `/camera/heartbeat` | Camera online heartbeat |
| GET | `/api/dashboard` | Mobile dashboard payload |
| GET | `/events` | Event history |
| GET | `/alerts` | Uploaded image list |
| GET | `/uploads/<filename>` | View an uploaded JPEG |
