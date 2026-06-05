import tempfile
from pathlib import Path
import unittest

import app as app_module


class SecurityServerTest(unittest.TestCase):
    def setUp(self):
        self.temp_dir = tempfile.TemporaryDirectory()
        root = Path(self.temp_dir.name)
        self.original_db_path = app_module.DB_PATH
        self.original_uploads_dir = app_module.UPLOADS_DIR
        app_module.DB_PATH = root / "test.db"
        app_module.UPLOADS_DIR = root / "uploads"
        app_module.UPLOADS_DIR.mkdir()
        app_module.initialize_database()
        app_module.app.config["TESTING"] = True
        self.client = app_module.app.test_client()

    def tearDown(self):
        app_module.DB_PATH = self.original_db_path
        app_module.UPLOADS_DIR = self.original_uploads_dir
        self.temp_dir.cleanup()

    def test_arm_and_disarm(self):
        self.assertEqual(self.client.post("/arm").json["system_status"], "ARMED")
        self.assertEqual(self.client.post("/disarm").json["system_status"], "DISARMED")

    def test_upload_creates_image_event(self):
        jpeg = b"\xff\xd8test-jpeg\xff\xd9"
        response = self.client.post(
            "/upload",
            data=jpeg,
            headers={"Content-Type": "image/jpeg", "X-Event-Type": "motion"},
        )
        self.assertEqual(response.status_code, 200)
        body = response.get_json()
        self.assertTrue(body["success"])
        self.assertTrue((app_module.UPLOADS_DIR / body["image"]).exists())

        dashboard = self.client.get("/api/dashboard").get_json()
        self.assertEqual(dashboard["events"][0]["event_type"], "motion")
        self.assertIsNotNone(dashboard["events"][0]["image_url"])

    def test_rejects_non_jpeg_upload(self):
        response = self.client.post("/upload", data=b"not-an-image")
        self.assertEqual(response.status_code, 400)

    def test_camera_heartbeat_sets_camera_online(self):
        self.assertEqual(self.client.get("/status").json["camera_status"], "Offline")
        self.assertEqual(self.client.post("/camera/heartbeat").status_code, 200)
        self.assertEqual(self.client.get("/status").json["camera_status"], "Online")

    def test_sensor_update_can_skip_duplicate_event(self):
        before = len(self.client.get("/events").json["events"])
        response = self.client.post(
            "/update_status",
            json={
                "event_type": "motion",
                "description": "Motion Detected",
                "log_event": False,
            },
        )
        self.assertEqual(response.status_code, 200)
        after = len(self.client.get("/events").json["events"])
        self.assertEqual(before, after)


if __name__ == "__main__":
    unittest.main()
