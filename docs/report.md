# Smart Home Security Project Report

## Completed stages

1. ESP8266 reads PIR and MC-38, drives blinking LED and active-low relay alarm.
2. ARM, DISARM, and STATUS are available through Telegram, web, and Flutter API.
3. ESP8266 sends immediate Telegram intrusion notifications.
4. ESP32-CAM exposes `/capture` and `/latest` without requiring microSD.
5. ESP32-CAM uploads JPEG images directly to Flask.
6. Telegram receives the alert and uploaded image link.
7. Flutter source provides dashboard, ARM/DISARM, photos, and history.
8. Flask SQLite stores timestamp, event type, description, and image filename.

## Storage model

```json
{
  "occurred_at": "2026-06-04T21:00:00+07:00",
  "event_type": "motion",
  "description": "Motion detected by PIR.",
  "image": "20260604_210000_000000_ab12cd34.jpg"
}
```

## Remaining deployment actions

- Flash both boards with the completed firmware.
- Replace `cameraCaptureUrl` with the ESP32-CAM IP printed on Serial Monitor.
- Allow inbound TCP port 5000 on the Flask server computer.
- Install Flutter SDK before building the Android application.
- Use `PUBLIC_BASE_URL` when Telegram image links must work outside the local WiFi.
