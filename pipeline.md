# Hướng Dẫn Xây Dựng Web Dashboard - Smart Home Security System

## 1. Mục tiêu

Xây dựng giao diện web cho phép người dùng:

* Theo dõi trạng thái hệ thống an ninh.
* Xem trạng thái cảm biến chuyển động (PIR).
* Xem trạng thái cửa (MC38).
* Bật/Tắt chế độ bảo vệ (ARM/DISARM).
* Xem lịch sử cảnh báo.
* Xem ảnh chụp từ ESP32-CAM.
* Nhận cảnh báo theo thời gian thực.

---

# 2. Kiến trúc hệ thống

```text
ESP8266
   |
   | HTTP / MQTT
   |
Flask Server
   |
   +---- SQLite
   |
   +---- Uploads
   |
Frontend Web
```

---

# 3. Công nghệ sử dụng

## Backend

* Python 3.12+
* Flask
* Flask-CORS
* SQLite
* Requests

## Frontend

* HTML5
* CSS3
* Bootstrap 5
* JavaScript
* Fetch API

## Thiết bị

* ESP8266
* ESP32-CAM
* PIR HC-SR501
* MC38 Door Sensor

---

# 4. Cấu trúc thư mục

```text
server/

├── app.py
├── database.db

├── uploads/

├── static/
│   ├── css/
│   │   └── style.css
│   │
│   ├── js/
│   │   └── app.js
│
├── templates/
│   ├── index.html
│   ├── history.html
│   └── gallery.html
│
└── api/
```

---

# 5. Các trang giao diện

## Dashboard

URL:

```text
/
```

Hiển thị:

### System Status

```text
ARMED
DISARMED
```

### Motion Sensor

```text
Motion Detected
No Motion
```

### Door Sensor

```text
Door Open
Door Closed
```

### Camera Status

```text
Online
Offline
```

---

## History Page

URL:

```text
/history
```

Hiển thị:

| Time  | Event     |
| ----- | --------- |
| 20:10 | Motion    |
| 20:15 | Door Open |

---

## Gallery Page

URL:

```text
/gallery
```

Hiển thị:

* Danh sách ảnh ESP32-CAM
* Thời gian chụp

---

# 6. Thiết kế Dashboard

```text
+------------------------------------+
| SMART HOME SECURITY SYSTEM         |
+------------------------------------+

System Status
[ ARMED ]

Door Status
[ CLOSED ]

Motion Status
[ NO MOTION ]

+-----------+-----------+
| ARM       | DISARM    |
+-----------+-----------+

Recent Events

20:10 Motion
20:15 Door Open

Camera Images
[ IMG ]
[ IMG ]
[ IMG ]
```

---

# 7. API Backend

## GET /status

Trả về trạng thái hiện tại.

Response:

```json
{
  "armed": true,
  "door": "closed",
  "motion": false
}
```

---

## POST /arm

Bật chế độ bảo vệ.

Response:

```json
{
  "success": true
}
```

---

## POST /disarm

Tắt chế độ bảo vệ.

Response:

```json
{
  "success": true
}
```

---

## POST /event

ESP8266 gửi sự kiện.

Request:

```json
{
  "event":"motion",
  "time":"2026-06-10 20:10"
}
```

---

## GET /history

Lấy lịch sử cảnh báo.

Response:

```json
[
  {
    "event":"motion",
    "time":"2026-06-10 20:10"
  }
]
```

---

# 8. Cơ sở dữ liệu

## Bảng events

```sql
CREATE TABLE events(
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    event TEXT,
    timestamp TEXT,
    image TEXT
);
```

---

# 9. Giao tiếp ESP8266

Khi phát hiện chuyển động:

```text
PIR Trigger
     |
     V
POST /event
```

Ví dụ:

```json
{
  "event":"motion"
}
```

---

Khi phát hiện mở cửa:

```json
{
  "event":"door_open"
}
```

---

# 10. Tích hợp ESP32-CAM

ESP32-CAM upload:

```text
/uploads/image001.jpg
/uploads/image002.jpg
```

Backend lưu:

```sql
image001.jpg
```

vào database.

---

# 11. Giai đoạn phát triển

## Version 1

* Dashboard
* PIR Status
* Door Status
* ARM/DISARM

## Version 2

* History Event
* SQLite

## Version 3

* ESP32-CAM
* Gallery

## Version 4

* Telegram Alert

## Version 5

* Real-time Update

---

# 12. Chức năng Demo Đồ Án

Người dùng nhấn:

```text
ARM
```

↓

ESP8266 chuyển sang chế độ bảo vệ.

↓

Mở cửa.

↓

Web hiển thị:

```text
Door Open
```

↓

Đi qua PIR.

↓

Web hiển thị:

```text
Motion Detected
```

↓

ESP32-CAM chụp ảnh.

↓

Ảnh xuất hiện trong Gallery.

↓

Telegram gửi cảnh báo tới điện thoại.

```
```
