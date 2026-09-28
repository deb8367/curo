import cv2
import numpy as np
import socket
import json
import math
import threading
import time

# ==========================================
# 1. NETWORK & CAMERA CONFIGURATION
# ==========================================
ESP32_IP = "192.168.1.150" # Replace with your ESP32's IP address
UDP_TX_PORT = 5006         # Send motor commands to ESP32
UDP_RX_PORT = 5005         # Receive ToF sensor data from ESP32

CAMERA_INDEX = 0           # 0 for default USB webcam
MARKER_ID = 0              # ArUco Marker ID on top of the car
SPEED = 180                # Default motor speed (0 - 255)

# Setup UDP Sockets
sock_tx = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock_rx = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock_rx.bind(("0.0.0.0", UDP_RX_PORT))
sock_rx.settimeout(0.02) # Non-blocking read

# Global shared variables
car_pose = None  # Stores (x_px, y_px, angle_rad)
mapped_points = [] # List of mapped obstacle coordinates (x, y)

# ==========================================
# 2. TOF DATA RECEIVER THREAD
# ==========================================
def udp_tof_listener():
    """Background thread to receive ToF sensor sweeps from ESP32."""
    global car_pose, mapped_points
    
    while True:
        try:
            data, _ = sock_rx.recvfrom(255)
            payload = json.loads(data.decode('utf-8'))
            
            servo_angle_deg = payload.get("a", 90)
            dist_mm = payload.get("d", 0)
            
            # Filter out invalid or out-of-range sensor readings
            if 50 < dist_mm < 2500 and car_pose is not None:
                car_x, car_y, car_heading = car_pose
                
                # Convert relative servo angle to radians (90 deg is straight ahead)
                relative_angle = math.radians(servo_angle_deg - 90)
                global_angle = car_heading + relative_angle
                
                # Calculate global obstacle coordinates in pixels
                dist_px = dist_mm * 0.15  # Scaling factor (adjust based on camera height)
                obs_x = int(car_x + dist_px * math.cos(global_angle))
                obs_y = int(car_y + dist_px * math.sin(global_angle))
                
                mapped_points.append((obs_x, obs_y))
                
                # Keep map memory manageable (last 500 points)
                if len(mapped_points) > 500:
                    mapped_points.pop(0)
                    
        except (socket.timeout, json.JSONDecodeError):
            continue

# Start UDP thread
threading.Thread(target=udp_tof_listener, daemon=True).start()

# ==========================================
# 3. MOTOR COMMAND HELPER
# ==========================================
def send_motor_command(left_speed, right_speed):
    """Sends PWM target values to the ESP32 car over UDP."""
    msg = json.dumps({"left": left_speed, "right": right_speed})
    sock_tx.sendto(msg.encode('utf-8'), (ESP32_IP, UDP_TX_PORT))

# ==========================================
# 4. MAIN VISION & CONTROL LOOP
# ==========================================
def main():
    global car_pose
    
    cap = cv2.VideoCapture(CAMERA_INDEX)
    cap.set(cv2.CAP_PROP_FRAME_WIDTH, 1280)
    cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 720)

    # Initialize OpenCV ArUco Detector
    dictionary = cv2.aruco.getPredefinedDictionary(cv2.aruco.DICT_4X4_50)
    parameters = cv2.aruco.DetectorParameters()
    
    # Compatibility fix for OpenCV 4.7+ vs older versions
    if hasattr(cv2.aruco, 'ArucoDetector'):
        detector = cv2.aruco.ArucoDetector(dictionary, parameters)
        detect_func = detector.detectMarkers
    else:
        detect_func = lambda frame: cv2.aruco.detectMarkers(frame, dictionary, parameters=parameters)

    print("\n--- CURO Pi 5 Vision Controller Active ---")
    print("Controls: W (Forward) | S (Backward) | A (Left) | D (Right) | Space (Stop) | Q (Quit)\n")

    while cap.isOpened():
        ret, frame = cap.read()
        if not ret:
            break

        # Detect ArUco markers in the frame
        corners, ids, _ = detect_func(frame)

        if ids is not None and MARKER_ID in ids.flatten():
            idx = np.where(ids.flatten() == MARKER_ID)[0][0]
            pts = corners[idx][0]

            # Calculate center coordinate (X, Y)
            center_x = int(np.mean(pts[:, 0]))
            center_y = int(np.mean(pts[:, 1]))

            # Calculate heading vector and orientation angle θ using top corners
            top_front_x = (pts[0][0] + pts[1][0]) / 2.0
            top_front_y = (pts[0][1] + pts[1][1]) / 2.0
            heading_rad = math.atan2(top_front_y - center_y, top_front_x - center_x)

            # Update shared car pose
            car_pose = (center_x, center_y, heading_rad)

            # Draw Car Tracking Overlay
            cv2.polylines(frame, [np.int32(pts)], True, (0, 255, 0), 2)
            cv2.circle(frame, (center_x, center_y), 5, (0, 0, 255), -1)
            
            # Draw Direction Line
            heading_line_x = int(center_x + 40 * math.cos(heading_rad))
            heading_line_y = int(center_y + 40 * math.sin(heading_rad))
            cv2.arrowedLine(frame, (center_x, center_y), (heading_line_x, heading_line_y), (255, 0, 0), 2)

        # Plot Mapped Obstacle Points from ESP32 ToF Sweeps
        for p in mapped_points:
            cv2.circle(frame, p, 2, (0, 255, 255), -1)

        # UI Diagnostics Overlay
        cv2.putText(frame, f"Mapped Points: {len(mapped_points)}", (20, 40),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 255), 2)

        cv2.imshow("CURO Pi 5 - Overhead Vision Map", frame)

        # Keyboard Controls Setup
        key = cv2.waitKey(1) & 0xFF
        if key == ord('w'):
            send_motor_command(SPEED, SPEED)     # Forward
        elif key == ord('s'):
            send_motor_command(-SPEED, -SPEED)   # Reverse
        elif key == ord('a'):
            send_motor_command(-SPEED, SPEED)    # Turn Left
        elif key == ord('d'):
            send_motor_command(SPEED, -SPEED)    # Turn Right
        elif key == ord(' '):
            send_motor_command(0, 0)             # Emergency Stop
        elif key == ord('q'):
            send_motor_command(0, 0)
            break

    cap.release()
    cv2.destroyAllWindows()

if __name__ == "__main__":
    main()
