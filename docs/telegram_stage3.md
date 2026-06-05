# Giai doan 3: Canh bao Telegram thoi gian thuc

Firmware ESP8266 gui canh bao Telegram khi he thong dang `ARMED` va:

- PIR chuyen sang trang thai phat hien chuyen dong.
- MC-38 chuyen sang trang thai cua mo.

Firmware khong gui tin khi dong cua, het chuyen dong, hoac he thong dang
`DISARMED`.

## 1. Tao bot va lay token

1. Mo Telegram, tim `@BotFather`.
2. Gui `/newbot` va lam theo huong dan.
3. Luu bot token do BotFather cung cap.
4. Mo bot vua tao va gui `/start`.

## 2. Lay Chat ID

Sau khi gui `/start`, mo URL sau tren trinh duyet:

```text
https://api.telegram.org/bot<BOT_TOKEN>/getUpdates
```

Tim gia tri `message.chat.id` trong ket qua JSON. Chat ID cua group thuong la
mot so am.

## 3. Cau hinh firmware

Mo file:

```text
test1_copy_20260604165126/test1_copy_20260604165126.ino
```

Thay hai gia tri:

```cpp
const char* telegramBotToken = "YOUR_BOT_TOKEN";
const char* telegramChatId = "YOUR_CHAT_ID";
```

## 4. Kiem thu

1. Nap firmware va mo Serial Monitor o `115200 baud`.
2. ARM he thong tu dashboard.
3. Mo cua hoac di qua PIR.
4. Serial Monitor phai hien `Telegram alert sent`.
5. Dien thoai nhan tin co bieu tuong canh bao, kem noi dung `Door Opened` hoac
   `Motion Detected`.

Neu Telegram tra ve loi `400`, kiem tra Chat ID. Neu tra ve `401`, kiem tra bot
token. ESP8266 va dien thoai khong can cung mang WiFi, nhung ESP8266 phai truy
cap duoc Internet.
