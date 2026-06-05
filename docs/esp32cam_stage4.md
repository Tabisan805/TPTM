# Giai doan 4: ESP32-CAM khong dung the microSD

Firmware:

```text
esp32cam/esp32cam.ino
```

ESP32-CAM chup anh va giu anh JPEG moi nhat trong RAM/PSRAM. Anh cu bi thay the
khi co lan chup moi va se mat khi ESP32-CAM khoi dong lai. Firmware hien tai
dong thoi upload JPEG len Flask Server, vi vay anh tren server duoc luu lau dai
ma khong can the microSD.

## API

- `GET /`: xem trang thai camera va thong tin anh trong RAM.
- `GET /capture`: chup anh moi va giu trong RAM.
- `GET /latest`: xem anh JPEG moi nhat.

Khi goi `/capture?event=motion` hoac `/capture?event=door_open`, camera upload
anh toi Flask `/upload` va tra ve URL anh tren server.

## Chuan bi

- AI Thinker ESP32-CAM.
- USB-to-TTL ho tro nguon 5V on dinh.
- ESP8266 va ESP32-CAM ket noi cung mang WiFi.

Khong can the microSD.

## Dau noi de nap firmware

| USB-to-TTL | ESP32-CAM |
| --- | --- |
| 5V | 5V |
| GND | GND |
| TX | U0R |
| RX | U0T |
| GND | GPIO0, chi noi khi nap |

Khong cap nguon ESP32-CAM qua chan 3.3V cua FTDI. Sau khi nap xong, thao day
noi `GPIO0-GND` va nhan reset.

## Cau hinh Arduino IDE

```text
Board: AI Thinker ESP32-CAM
Port: cong COM cua USB-to-TTL
Upload Speed: 115200
Partition Scheme: Huge APP
Serial Monitor: 115200 baud
```

## Lay IP va thu camera

Sau khi nap firmware, thao `GPIO0-GND`, nhan reset va xem Serial Monitor:

```text
ESP32-CAM IP: 192.168.0.xxx
ESP32-CAM API ready
```

Mo cac URL sau tren may tinh cung mang WiFi:

```text
http://192.168.0.xxx/
http://192.168.0.xxx/capture
http://192.168.0.xxx/latest
```

Sau khi goi `/capture`, endpoint `/latest` hien thi anh vua chup.

## Cau hinh ESP8266

Trong firmware ESP8266, thay IP tai:

```cpp
const char* cameraCaptureUrl = "http://192.168.0.102/capture";
```

bang IP ESP32-CAM in tren Serial Monitor, sau do nap lai ESP8266.

## Demo toan bo

1. Khoi dong ESP32-CAM va xac nhan `/capture` hoat dong.
2. ARM he thong.
3. Mo cua hoac di qua PIR.
4. Telegram nhan canh bao.
5. ESP8266 in `ESP32-CAM captured a JPEG`.
6. Mo `http://<ESP32-CAM-IP>/latest` de xem anh.

Neu `/capture` bao khong du RAM, kiem tra dung board AI Thinker ESP32-CAM va
PSRAM da duoc bat. Neu camera lien tuc khoi dong lai, dung nguon 5V on dinh hon.
