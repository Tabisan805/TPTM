# Smart Home Security Flutter App

The app reads the Flask API, shows system/sensor/camera status, sends ARM and
DISARM commands, displays event history, and displays uploaded camera images.

Flutter SDK is not installed on the current development machine. On a machine
with Flutter installed, run:

```powershell
powershell -ExecutionPolicy Bypass -File mobile/flutter_app/bootstrap.ps1
cd mobile/flutter_app
flutter run
```

The app defaults to:

```text
http://192.168.0.101:5000
```

The server URL can be changed from the text field at the top of the app. The
bootstrap script generates the Android project and enables local HTTP access.
