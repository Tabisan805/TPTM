from contextlib import contextmanager
from datetime import datetime
import os
from pathlib import Path
import sqlite3
from uuid import uuid4

from flask import Flask, jsonify, render_template, request, send_from_directory, url_for
from flask_cors import CORS

BASE_DIR = Path(__file__).parent
DB_PATH = BASE_DIR / "database.db"
UPLOADS_DIR = BASE_DIR / "static" / "uploads"
UPLOADS_DIR.mkdir(parents=True, exist_ok=True)

app = Flask(__name__, template_folder="templates", static_folder="static")
app.config["MAX_CONTENT_LENGTH"] = 5 * 1024 * 1024
CORS(app)


def now_iso():
    return datetime.now().astimezone().isoformat(timespec="seconds")


def get_db_connection():
    conn = sqlite3.connect(DB_PATH)
    conn.row_factory = sqlite3.Row
    return conn


@contextmanager
def database_connection():
    conn = get_db_connection()
    try:
        yield conn
    finally:
        conn.close()


def initialize_database():
    with database_connection() as conn:
        conn.execute(
            """
            CREATE TABLE IF NOT EXISTS system_state (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                system_status TEXT NOT NULL,
                motion_status TEXT NOT NULL,
                door_status TEXT NOT NULL,
                camera_status TEXT NOT NULL,
                updated_at TEXT NOT NULL
            )
            """
        )
        conn.execute(
            """
            CREATE TABLE IF NOT EXISTS events (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                occurred_at TEXT NOT NULL,
                event_type TEXT NOT NULL,
                description TEXT NOT NULL,
                image TEXT
            )
            """
        )
        conn.execute(
            """
            CREATE TABLE IF NOT EXISTS device_heartbeat (
                device TEXT PRIMARY KEY,
                last_seen TEXT NOT NULL
            )
            """
        )

        event_columns = {
            row["name"] for row in conn.execute("PRAGMA table_info(events)").fetchall()
        }
        if "image" not in event_columns:
            conn.execute("ALTER TABLE events ADD COLUMN image TEXT")

        state_count = conn.execute("SELECT COUNT(*) FROM system_state").fetchone()[0]
        if state_count == 0:
            conn.execute(
                """
                INSERT INTO system_state
                    (system_status, motion_status, door_status, camera_status, updated_at)
                VALUES (?, ?, ?, ?, ?)
                """,
                ("DISARMED", "No Motion", "Door Closed", "Offline", now_iso()),
            )
            conn.execute(
                """
                INSERT INTO events (occurred_at, event_type, description, image)
                VALUES (?, ?, ?, NULL)
                """,
                (now_iso(), "system_ready", "Smart security server initialized."),
            )
        conn.commit()


def read_system_state():
    with database_connection() as conn:
        row = conn.execute(
            "SELECT * FROM system_state ORDER BY id DESC LIMIT 1"
        ).fetchone()
        heartbeat = conn.execute(
            "SELECT last_seen FROM device_heartbeat WHERE device = 'camera'"
        ).fetchone()

    state = dict(row) if row else {
        "system_status": "DISARMED",
        "motion_status": "No Motion",
        "door_status": "Door Closed",
        "camera_status": "Offline",
        "updated_at": now_iso(),
    }
    state["camera_status"] = "Offline"
    if heartbeat:
        last_seen = datetime.fromisoformat(heartbeat["last_seen"])
        age = datetime.now().astimezone() - last_seen
        if age.total_seconds() < 90:
            state["camera_status"] = "Online"
    return state


def save_system_state(state):
    with database_connection() as conn:
        conn.execute(
            """
            INSERT INTO system_state
                (system_status, motion_status, door_status, camera_status, updated_at)
            VALUES (?, ?, ?, ?, ?)
            """,
            (
                state["system_status"],
                state["motion_status"],
                state["door_status"],
                state["camera_status"],
                now_iso(),
            ),
        )
        conn.commit()


def add_event(event_type, description, image=None):
    with database_connection() as conn:
        cursor = conn.execute(
            """
            INSERT INTO events (occurred_at, event_type, description, image)
            VALUES (?, ?, ?, ?)
            """,
            (now_iso(), event_type, description, image),
        )
        conn.commit()
        return cursor.lastrowid


def record_camera_heartbeat():
    with database_connection() as conn:
        conn.execute(
            """
            INSERT INTO device_heartbeat (device, last_seen) VALUES ('camera', ?)
            ON CONFLICT(device) DO UPDATE SET last_seen = excluded.last_seen
            """,
            (now_iso(),),
        )
        conn.commit()


def public_image_url(filename):
    path = url_for("uploaded_image", filename=filename)
    base_url = os.getenv("PUBLIC_BASE_URL", "").rstrip("/")
    if base_url:
        return f"{base_url}{path}"
    return request.host_url.rstrip("/") + path


def serialize_event(row):
    event = dict(row)
    event["image_url"] = public_image_url(event["image"]) if event.get("image") else None
    return event


def read_events(limit=50):
    limit = max(1, min(limit, 200))
    with database_connection() as conn:
        rows = conn.execute(
            """
            SELECT id, occurred_at, event_type, description, image
            FROM events ORDER BY id DESC LIMIT ?
            """,
            (limit,),
        ).fetchall()
    return [serialize_event(row) for row in rows]


def scan_alert_images(limit=100):
    images = []
    paths = sorted(
        UPLOADS_DIR.iterdir(),
        key=lambda path: path.stat().st_mtime,
        reverse=True,
    )
    for path in paths[:limit]:
        if path.is_file() and path.suffix.lower() in {".jpg", ".jpeg", ".png"}:
            stat = path.stat()
            images.append(
                {
                    "filename": path.name,
                    "url": public_image_url(path.name),
                    "modified": datetime.fromtimestamp(stat.st_mtime).astimezone().isoformat(
                        timespec="seconds"
                    ),
                    "size": stat.st_size,
                }
            )
    return images


def infer_status_from_event(event_type, description=""):
    event_type = (event_type or "").lower().strip()
    description = (description or "").lower()
    text = f"{event_type} {description}"
    update = {}

    if "door" in text:
        update["door_status"] = (
            "Door Closed" if "close" in text else "Door Open"
        )
    if "motion" in text:
        update["motion_status"] = (
            "No Motion"
            if any(word in text for word in ("end", "stopped", "no motion"))
            else "Motion Detected"
        )
    return update


def description_for_event(event_type):
    descriptions = {
        "motion": "Motion detected by PIR.",
        "door_open": "Door opened while security system armed.",
        "door_close": "Door closed.",
        "motion_end": "Motion ended.",
        "intrusion": "Intrusion detected.",
    }
    return descriptions.get(event_type, f"Camera captured event: {event_type}.")


@app.route("/")
def dashboard():
    return render_template("index.html")


@app.route("/history")
def history_page():
    return render_template("history.html")


@app.route("/gallery")
def gallery_page():
    return render_template("gallery.html")


@app.route("/health")
def health():
    return jsonify({"success": True, "time": now_iso()})


@app.route("/status")
def get_status():
    state = read_system_state()
    events = read_events(1)
    state["last_event"] = events[0] if events else {}
    return jsonify(state)


@app.route("/api/dashboard")
def api_dashboard():
    limit = request.args.get("limit", 20, type=int)
    return jsonify(
        {
            "status": read_system_state(),
            "events": read_events(limit),
            "images": scan_alert_images(limit),
        }
    )


@app.route("/events")
def get_events():
    return jsonify({"events": read_events(request.args.get("limit", 50, type=int))})


@app.route("/alerts")
def get_alerts():
    return jsonify({"alerts": scan_alert_images(request.args.get("limit", 100, type=int))})


@app.route("/uploads/<path:filename>")
def uploaded_image(filename):
    return send_from_directory(UPLOADS_DIR, filename)


@app.route("/upload", methods=["POST"])
def upload_image():
    image_data = request.get_data()
    if len(image_data) < 4 or not image_data.startswith(b"\xff\xd8"):
        return jsonify({"success": False, "error": "Request body must be a JPEG image"}), 400

    event_type = (
        request.headers.get("X-Event-Type")
        or request.args.get("event")
        or "intrusion"
    ).strip().lower()[:50]
    description = (
        request.headers.get("X-Description") or description_for_event(event_type)
    ).strip()[:300]

    timestamp = datetime.now().astimezone().strftime("%Y%m%d_%H%M%S_%f")
    filename = f"{timestamp}_{uuid4().hex[:8]}.jpg"
    (UPLOADS_DIR / filename).write_bytes(image_data)

    state = read_system_state()
    state.update(infer_status_from_event(event_type, description))
    state["camera_status"] = "Online"
    save_system_state(state)
    record_camera_heartbeat()
    event_id = add_event(event_type, description, filename)

    return jsonify(
        {
            "success": True,
            "event_id": event_id,
            "event": event_type,
            "image": filename,
            "image_url": public_image_url(filename),
            "bytes": len(image_data),
            "time": now_iso(),
        }
    )


@app.route("/update_status", methods=["GET", "POST"])
def update_status():
    if request.method == "GET":
        return jsonify(
            {
                "info": "POST motion_status, door_status, event_type and description",
                "success": True,
            }
        )

    data = request.get_json(silent=True) or {}
    if not data:
        return jsonify({"success": False, "error": "Invalid JSON payload"}), 400

    state = read_system_state()
    event_type = str(data.get("event_type") or "sensor")[:50]
    description = str(data.get("description") or "")[:300]
    state.update(infer_status_from_event(event_type, description))
    if data.get("motion_status") is not None:
        state["motion_status"] = str(data["motion_status"])[:50]
    if data.get("door_status") is not None:
        state["door_status"] = str(data["door_status"])[:50]
    save_system_state(state)
    if parse_bool(data.get("log_event", True)):
        add_event(event_type, description)
    return jsonify({"success": True, "status": state})


def parse_bool(value):
    if isinstance(value, bool):
        return value
    if isinstance(value, str):
        return value.lower() in {"1", "true", "yes", "on"}
    return bool(value)


@app.route("/sensor", methods=["POST"])
def sensor_update():
    data = request.get_json(silent=True) or {}
    if "motion" not in data and "door_open" not in data:
        return jsonify({"success": False, "error": "Include motion or door_open"}), 400

    state = read_system_state()
    if "motion" in data:
        state["motion_status"] = (
            "Motion Detected" if parse_bool(data["motion"]) else "No Motion"
        )
    if "door_open" in data:
        state["door_status"] = (
            "Door Open" if parse_bool(data["door_open"]) else "Door Closed"
        )
    save_system_state(state)
    return jsonify({"success": True, "status": state})


@app.route("/camera/heartbeat", methods=["POST"])
def camera_heartbeat():
    record_camera_heartbeat()
    return jsonify({"success": True})


@app.route("/arm", methods=["POST"])
def arm_system():
    state = read_system_state()
    state["system_status"] = "ARMED"
    save_system_state(state)
    add_event("arm", "Security system armed.")
    return jsonify(state)


@app.route("/disarm", methods=["POST"])
def disarm_system():
    state = read_system_state()
    state["system_status"] = "DISARMED"
    save_system_state(state)
    add_event("disarm", "Security system disarmed.")
    return jsonify(state)


initialize_database()

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000, debug=True)
