# curo
Low-cost indoor autonomous mapping system using an ESP32 ToF micro-car tracked by a Raspberry Pi 5 overhead vision network.
┌──────────────────────┐                     ┌───────────────────────────┐
 │ Table-Mounted Webcam │ ──[USB Video Feed]─>│     Raspberry Pi 5        │
 └──────────────────────┘                     │        (vis.py)           │
                                              │ - ArUco Marker Tracker    │
 ┌──────────────────────┐                     │ - Coordinate Transformer  │
 │  ESP32 Micro-Car     │ <──[UDP Wi-Fi]─────>│ - Map Generator           │
 │      (esp.ino)       │  Commands & Telemetry - Keyboard Controller    │
 └──────────────────────┘                     └───────────────────────────┘
 vis.py (Raspberry Pi 5): Tracks an ArUco marker mounted on top of the car via an overhead camera to determine global position $(X, Y, \theta)$. Receives ToF scans via UDP, transforms relative range measurements to global map coordinates, renders a live visualization overlay, and broadcasts motor telemetry.esp.ino (ESP32): Drives a 2WD chassis, controls a servo to sweep a Time-of-Flight (ToF) distance sensor, streams range scans over Wi-Fi UDP to the Pi 5, and executes incoming motor speed commands.
 
 
 
  File StructureFileEnvironmentDescriptionesp.inoArduino / ESP32Firmware for motor driving, servo sweeping, ToF distance reading, and low-latency UDP networking.vis.pyPython 3 (Pi 5)Overhead vision engine, global coordinate math, real-time map plotting with OpenCV, and teleoperation keyboard interface.🛠️ Hardware RequirementsCentral Unit: Raspberry Pi 5 + HD USB Webcam (1080p recommended).Mobile Unit (CURO Rover):ESP32 Development Board (e.g., ESP32-WROOM-32 or ESP32-S3).VL53L1X Time-of-Flight Distance Sensor.Micro Servo Motor (SG90).Dual Motor Driver Module (TB6612FNG or L298N).2WD Smart Robot Car Chassis + DC Motors.Printed ArUco Marker (DICT_4X4_50, Marker ID: 0) attached flat on top of the car.Power Source: 7.4V LiPo Battery or dual 18650 cells with 5V step-down regulator.
