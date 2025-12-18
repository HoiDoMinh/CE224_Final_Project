import cv2
import time
from ultralytics import YOLO
import paho.mqtt.client as mqtt
import ssl

# CONFIGURATION
CONFIDENCE_THRESHOLD = 0.5
FIRE_ALERT_THRESHOLD = 3
ALERT_TIME_LIMIT = 4
NO_FIRE_CONFIRM_TIME = 10

MQTT_SERVER = "c4c3e7f0542c4c4e99afe483bac974e6.s1.eu.hivemq.cloud"
MQTT_PORT = 8883
MQTT_USERNAME = "dominhhoi123"
MQTT_PASSWORD = "#gtCEUS4AnG!qHm"
MQTT_TOPIC = "esp32cam/fire_detection"

# BIEN TOÀN CỤC
fire_count = 0
smoke_count = 0
start_time = None
last_fire_time = None
previous_alert_state = False

# MQTT SETUP
mqtt_client = mqtt.Client()
mqtt_client.username_pw_set(MQTT_USERNAME, MQTT_PASSWORD)
mqtt_client.tls_set(tls_version=ssl.PROTOCOL_TLS)

try:
    mqtt_client.connect(MQTT_SERVER, MQTT_PORT, 60)
    mqtt_client.loop_start()
except Exception as e:
    print(f"Lỗi MQTT: {e}")

# MAIN
def main():
    global fire_count, smoke_count, start_time, last_fire_time, previous_alert_state

    #url = "http://192.168.1.174:81/stream"
    url = "http://10.173.150.131:81/stream"
    cap = cv2.VideoCapture(url)

    if not cap.isOpened():
        print("Không kết nối được camera")
        return

    model = YOLO("D:\\DATA\\DOAN\\HE_THONG_NHUNG\\ADRUINO\\best_320.pt")
    counter = 0
    frame_counter = 0
    fps = 0
    fps_timer = time.time()

    while True:
        ret, frame = cap.read()
        if not ret:
            continue

        results = model(frame, conf=CONFIDENCE_THRESHOLD, verbose=False)[0]
        annotated_frame = frame.copy()
        
        fire_detected_this_frame = False
        smoke_detected_this_frame = False

        # Xử lý detection
        for result in results:
            for box in result.boxes:
                class_id = int(box.cls[0])
                conf = box.conf[0].item()
                label = "fire" if class_id == 1 else "smoke"

                if conf >= CONFIDENCE_THRESHOLD:
                    if label == "fire":
                        fire_detected_this_frame = True
                    elif label == "smoke":
                        smoke_detected_this_frame = True

                    x1, y1, x2, y2 = map(int, box.xyxy[0])
                    color = (0, 0, 255) if label == "fire" else (0, 255, 255)
                    cv2.rectangle(annotated_frame, (x1, y1), (x2, y2), color, 2)
                    cv2.putText(annotated_frame, f"{label} {conf:.2f}", (x1, y1 - 10),
                                cv2.FONT_HERSHEY_SIMPLEX, 0.7, color, 2)

        # Cập nhật bộ đếm
        if fire_detected_this_frame:
            last_fire_time = time.time()
            if fire_count == 0:
                start_time = time.time()
            fire_count += 1
            smoke_count = 0
        elif smoke_detected_this_frame:
            smoke_count += 1
            if fire_count > 0:
                fire_count -= 1
        else:
            if fire_count > 0:
                fire_count -= 1
            if smoke_count > 0:
                smoke_count -= 1

        # Reset nếu quá lâu
        if not fire_detected_this_frame and start_time is not None:
            if time.time() - start_time > ALERT_TIME_LIMIT:
                fire_count = 0
                start_time = None

        # Xác định trạng thái
        current_alert_state = fire_count >= FIRE_ALERT_THRESHOLD
        now = time.time()

        # Gửi MQTT khi trạng thái thay đổi
        if current_alert_state:
            last_fire_time = now
            cv2.putText(annotated_frame, "FIRE ALERT!", (50, 100),
                        cv2.FONT_HERSHEY_SIMPLEX, 1.5, (0, 0, 255), 3)

            if current_alert_state != previous_alert_state:
                mqtt_client.publish(MQTT_TOPIC, "1", qos=1)
                print(f"CẢNH BÁO CHÁY | Fire: {fire_count}, Smoke: {smoke_count}")
            previous_alert_state = True

        elif previous_alert_state and last_fire_time is not None and (now - last_fire_time > NO_FIRE_CONFIRM_TIME):
            mqtt_client.publish(MQTT_TOPIC, "0", qos=1)
            print(f"HẾT CHÁY sau {NO_FIRE_CONFIRM_TIME}s")
            previous_alert_state = False

        # Hiển thị thông tin
        status = "FIRE" if current_alert_state else "NORMAL"
        cv2.putText(annotated_frame, f"Fire count: {fire_count}", 
                    (10, 60), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 255), 2)
        cv2.putText(annotated_frame, status, (10, 90),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 0, 255) if current_alert_state else (0, 255, 0), 2)

        # FPS
        frame_counter += 1
        if time.time() - fps_timer >= 1.0:
            fps = frame_counter / (time.time() - fps_timer)
            frame_counter = 0
            fps_timer = time.time()

        cv2.putText(annotated_frame, f"FPS: {fps:.1f}", (10, 30),
            cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 0), 2)

        cv2.imshow("Fire Detection", annotated_frame)

        # Xử lý phím
        key = cv2.waitKey(1) & 0xFF
        if key == ord('q'):
            break
        elif key == ord('c'):
            cv2.imwrite(f"capture_{counter}.jpg", annotated_frame)
            print(f"Đã lưu: capture_{counter}.jpg")
            counter += 1

    # Cleanup
    mqtt_client.loop_stop()
    mqtt_client.disconnect()
    cap.release()
    cv2.destroyAllWindows()

if __name__ == "__main__":
    main()