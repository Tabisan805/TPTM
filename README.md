<h2 align="center">
    <a href="https://dainam.edu.vn/vi/khoa-cong-nghe-thong-tin">
    🎓 Faculty of Information Technology (DaiNam University)
    </a>
</h2>
<h2 align="center">
   HỆ THỐNG CẢNH BÁO THỜI GIAN THỰC
</h2>
<div align="center">
    <p align="center">
        <img src="docs/aiotlab_logo.png" alt="AIoTLab Logo" width="170"/>
        <img src="docs/fitdnu_logo.png" alt="AIoTLab Logo" width="180"/>
        <img src="docs/dnu_logo.png" alt="DaiNam University Logo" width="200"/>
    </p>

[![AIoTLab](https://img.shields.io/badge/AIoTLab-green?style=for-the-badge)](https://www.facebook.com/DNUAIoTLab)
[![Faculty of Information Technology](https://img.shields.io/badge/Faculty%20of%20Information%20Technology-blue?style=for-the-badge)](https://dainam.edu.vn/vi/khoa-cong-nghe-thong-tin)
[![DaiNam University](https://img.shields.io/badge/DaiNam%20University-orange?style=for-the-badge)](https://dainam.edu.vn)

</div>
# Smart Home Security System

Dự án mô phỏng hệ thống an ninh nhà thông minh gồm Flask server, web dashboard, ứng dụng Flutter và ESP32-CAM. Hệ thống cho phép bật/tắt bảo vệ, theo dõi trạng thái cảm biến, nhận ảnh cảnh báo từ camera và xem lịch sử sự kiện.

## Thành phần

- `server/`: Flask API, web dashboard, SQLite runtime database.
- `mobile/flutter_app/`: ứng dụng Flutter đọc dữ liệu từ Flask API.
- `esp32cam/`: firmware ESP32-CAM chụp ảnh, upload ảnh và gửi heartbeat.
- `docs/`: tài liệu kiến trúc, hướng dẫn hệ thống và các giai đoạn triển khai.
- `test1_copy_20260604165126/`: sketch Arduino/ESP thử nghiệm.

## Yêu cầu

- Python 3.12+.
- Flutter SDK 3.3+ nếu chạy mobile app.
- Arduino IDE hoặc PlatformIO nếu nạp code ESP32-CAM.

## Chạy Flask server

Tạo môi trường ảo và cài dependency:

```powershell
python -m venv .venv
.\.venv\Scripts\python.exe -m pip install -r server\requirements.txt
```

Chạy server:

```powershell
.\.venv\Scripts\python.exe server\app.py
```

Hoặc dùng script có sẵn:

```powershell
powershell -ExecutionPolicy Bypass -File server\run_server.ps1
```

Server mặc định chạy tại:

```text
http://localhost:5000
```

Các trang web:

- Dashboard: `http://localhost:5000/`
- History: `http://localhost:5000/history`
- Gallery: `http://localhost:5000/gallery`

## API chính

- `GET /health`: kiểm tra server.
- `GET /status`: lấy trạng thái hệ thống hiện tại.
- `GET /api/dashboard?limit=20`: lấy trạng thái, sự kiện và ảnh mới nhất.
- `GET /events?limit=50`: lấy lịch sử sự kiện.
- `GET /alerts?limit=100`: lấy danh sách ảnh cảnh báo.
- `POST /arm`: bật chế độ bảo vệ.
- `POST /disarm`: tắt chế độ bảo vệ.
- `POST /sensor`: cập nhật trạng thái cảm biến bằng JSON.
- `POST /update_status`: cập nhật trạng thái/sự kiện bằng JSON.
- `POST /upload`: upload ảnh JPEG từ ESP32-CAM.
- `POST /camera/heartbeat`: cập nhật trạng thái online của camera.

Ví dụ cập nhật cảm biến:

```powershell
Invoke-RestMethod -Method Post -Uri http://localhost:5000/sensor `
  -ContentType "application/json" `
  -Body '{"motion":true,"door_open":false}'
```

## Chạy test backend

```powershell
cd server
..\.venv\Scripts\python.exe -m unittest test_app.py
```

Nếu đang ở thư mục gốc project, dùng:

```powershell
.\.venv\Scripts\python.exe -m unittest discover -s server
```

## Chạy Flutter app

Trên máy đã cài Flutter:

```powershell
powershell -ExecutionPolicy Bypass -File mobile\flutter_app\bootstrap.ps1
cd mobile\flutter_app
flutter run
```

App mặc định gọi server tại:

```text
http://192.168.0.101:5000
```

Có thể đổi URL server trực tiếp trong ô nhập ở đầu ứng dụng.

## ESP32-CAM

Copy file cấu hình mẫu:

```powershell
Copy-Item esp32cam\wifi_config.example.h esp32cam\wifi_config.h
```

Mở `esp32cam/wifi_config.h`, chỉnh các giá trị sau cho đúng mạng nội bộ:

```cpp
const char* ssid = "YOUR_WIFI";
const char* password = "YOUR_PASSWORD";
const char* uploadUrl = "http://YOUR_SERVER_IP:5000/upload";
const char* heartbeatUrl = "http://YOUR_SERVER_IP:5000/camera/heartbeat";
```

Sau đó nạp sketch cho board ESP32-CAM AI Thinker.

`esp32cam/wifi_config.h` đã được ignore để tránh commit WiFi/password thật.

Với sketch thử nghiệm trong `test1_copy_20260604165126/`, copy file mẫu trước khi nạp:

```powershell
Copy-Item test1_copy_20260604165126\secrets_config.example.h test1_copy_20260604165126\secrets_config.h
```

Sau đó chỉnh WiFi, Flask server, ESP32-CAM URL và Telegram token trong `secrets_config.h`. File này cũng đã được ignore.

Trước khi public repository, nên kiểm tra lại các file firmware để không đẩy WiFi/password thật hoặc địa chỉ IP nội bộ nhạy cảm.
