# CURO

Low-cost indoor autonomous mapping platform built around a compact ESP32-based mobile robot and a Raspberry Pi 5 overhead vision system. The robot localizes itself using an ArUco marker, scans its surroundings with a VL53L1X ToF sensor, and converts the measurements into a real-time map of the environment.

## Overview

CURO combines low-cost hardware with a lightweight perception and control pipeline to enable indoor navigation and mapping without expensive localization hardware. The Raspberry Pi tracks the robot pose from a top-down camera and the ESP32 handles low-level motion control and sensor acquisition.

```text
┌──────────────────────┐     USB Video Feed     ┌──────────────────────────────┐
│ Table-Mounted Camera │ ───────────────────▶ │ Raspberry Pi 5              │
└──────────────────────┘                     │  - vis.py                   │
                                            │  - ArUco marker tracking    │
                                            │  - Pose estimation          │
                                            │  - Coordinate transform     │
                                            │  - Map generation           │
                                            └──────────────┬───────────────┘
                                                           │ UDP / Wi‑Fi
                                                           ▼
┌──────────────────────┐  Motor Commands & Telemetry  ┌──────────────────────────────┐
│ ESP32 Micro-Car      │ ◀─────────────────────────────▶ │ ToF + Servo System          │
│  (esp.ino)          │                                │  - VL53L1X distance sensor  │
└──────────────────────┘                                │  - Servo sweep              │
                                                       │  - Obstacle sensing         │
                                                       └──────────────────────────────┘
```

## System Architecture

### Raspberry Pi 5 (`vis.py`)
- Detects and tracks an ArUco marker mounted on the robot
- Estimates robot pose in image coordinates as (x, y, θ)
- Receives ToF scans from the ESP32 over UDP
- Transforms relative sensor readings into global coordinates
- Generates and displays a 2D obstacle map in real time
- Accepts keyboard commands for manual driving and testing

### ESP32 Robot (`esp.ino`)
- Drives the two-wheel chassis using motor control logic
- Sweeps a ToF sensor across a field of view
- Reads distance measurements from the VL53L1X sensor
- Sends telemetry packets back to the Raspberry Pi
- Receives speed commands from the Pi over UDP

## Key Features

- Low-cost indoor autonomous mapping
- Overhead camera-based localization and heading estimation
- Real-time ToF obstacle scanning with a rotating sensor head
- Conversion from robot-relative measurements to global map coordinates
- Wireless control and telemetry over Wi‑Fi UDP
- Simple interaction via keyboard interface for testing and development

## Project Structure

| File | Environment | Description |
| --- | --- | --- |
| `esp.ino` | Arduino / ESP32 | Firmware for motor driving, servo control, ToF measurements, and UDP networking |
| `vis.py` | Python 3 / Raspberry Pi 5 | Overhead vision processing, ArUco tracking, pose estimation, map generation, and keyboard control |

## Hardware Requirements

- Raspberry Pi 5
- USB webcam or camera module
- ESP32 development board
- 2-wheel mobile robot chassis
- VL53L1X ToF distance sensor
- Servo motor for sensor sweep
- Motor driver circuit
- ArUco marker mounted on the robot
- Wi‑Fi network for communication between the Pi and ESP32

## Setup

### 1. Configure the ESP32
Edit the following values in `esp.ino` before uploading:

- `WIFI_SSID`
- `WIFI_PASSWORD`
- `PI5_IP_ADDRESS`
- UDP port settings if needed

### 2. Configure the Pi
Update the network settings in `vis.py`:

- `ESP32_IP`
- `UDP_TX_PORT`
- `UDP_RX_PORT`

### 3. Run the vision system
```bash
python3 vis.py
```

### 4. Control the robot
Use the keyboard controls:

- `W` — move forward
- `S` — reverse
- `A` — turn left
- `D` — turn right
- `Space` — stop
- `Q` — quit

## How It Works

1. The overhead camera tracks the robot using an ArUco marker.
2. The Pi estimates the robot’s position and heading in the global frame.
3. The ESP32 sweeps the ToF sensor across multiple angles and emits distance readings.
4. Each measurement is transformed into world coordinates using the robot pose and sensor angle.
5. The resulting points are plotted to form an obstacle map in real time.

## Notes

- The scaling factor used to convert range measurements to map pixels may need calibration depending on camera height and floor geometry.
- Network IP addresses must match the actual devices on the local network.
- The system is intended as a lightweight research and prototyping platform, and can be extended for navigation, path planning, or SLAM-based behavior.

## License

This project is provided as-is for educational and prototyping use.

## Repository Status

CURO demonstrates an affordable approach to indoor mapping and localization using a low-cost robot platform, overhead vision tracking, and ToF sensing.
