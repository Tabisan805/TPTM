# Alarm Flow

```text
DISARMED
   |
   | ARM from Web, Flutter, or Telegram
   v
ARMED
   |
   | PIR motion or door opened
   v
ALARM
   |-- Relay ON
   |-- LED blinking
   |-- Telegram text alert
   |-- ESP32-CAM capture
   |-- Flask JPEG upload + SQLite event
   `-- Telegram image link
   |
   | DISARM
   v
DISARMED: Relay OFF + LED OFF
```
