#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include <Adafruit_VL53L1X.h>
#include <ESP32Servo.h>
#include <ArduinoJson.h>

// ==========================================
// 1. CONFIGURATION & PIN MAPPINGS
// ==========================================
const char* WIFI_SSID     = "YOUR_WIFI_NAME";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

// Target Raspberry Pi 5 IP & UDP Ports
const char* PI5_IP_ADDRESS = "192.168.1.100"; 
const int UDP_TX_PORT      = 5005; // Port to send ToF data to Pi 5
const int UDP_RX_PORT      = 5006; // Port to listen for motor commands from Pi 5

// Pin Definitions
#define SERVO_PIN      18  // Servo PWM pin
#define I2C_SDA        21  // VL53L1X SDA
#define I2C_SCL        22  // VL53L1X SCL

// Motor Driver Pins (TB6612FNG / L298N equivalent)
#define MOTOR_A_IN1    26
#define MOTOR_A_IN2    27
#define MOTOR_A_PWM    14

#define MOTOR_B_IN1    32
#define MOTOR_B_IN2    33
#define MOTOR_B_PWM    25

// ==========================================
// 2. GLOBAL OBJECTS
// ==========================================
WiFiUDP udp;
Adafruit_VL53L1X vl53;
Servo tofServo;

int currentAngle = 90;
int sweepDirection = 5; // Degrees per step

// PWM Config for ESP32 LEDC (Motor Speed Control)
const int PWM_FREQ = 5000;
const int PWM_RES  = 8; // 8-bit resolution (0-255)
const int CH_A     = 0;
const int CH_B     = 1;

// Function Declarations
void connectToWiFi();
void readAndSendToFData();
void handleIncomingMotorCommands();
void setMotorSpeeds(int leftSpeed, int rightSpeed);

void setup() {
  Serial.begin(115200);
  Wire.begin(I2C_SDA, I2C_SCL);

  // Setup Motors
  pinMode(MOTOR_A_IN1, OUTPUT);
  pinMode(MOTOR_A_IN2, OUTPUT);
  pinMode(MOTOR_B_IN1, OUTPUT);
  pinMode(MOTOR_B_IN2, OUTPUT);

  ledcSetup(CH_A, PWM_FREQ, PWM_RES);
  ledcAttachPin(MOTOR_A_PWM, CH_A);
  
  ledcSetup(CH_B, PWM_FREQ, PWM_RES);
  ledcAttachPin(MOTOR_B_PWM, CH_B);

  setMotorSpeeds(0, 0); // Stop initially

  // Setup Servo
  tofServo.attach(SERVO_PIN);
  tofServo.write(currentAngle);

  // Setup ToF Sensor
  if (!vl53.begin(0x29, &Wire)) {
    Serial.println(F("Failed to detect VL53L1X sensor!"));
    while (1) delay(10);
  }
  vl53.startRanging();
  vl53.setTimingBudget(33); // Fast 33ms measurement budget

  // Connect Network
  connectToWiFi();
  udp.begin(UDP_RX_PORT);
  Serial.println("CURO ESP32 Node Ready!");
}

void loop() {
  readAndSendToFData();
  handleIncomingMotorCommands();
  delay(20); // Control loop delay (~50Hz refresh)
}

// ==========================================
// 3. NETWORK & HELPER FUNCTIONS
// ==========================================

void connectToWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected! IP Address: " + WiFi.localIP().toString());
}

// Sweeps ToF sensor, measures distance, and sends JSON packet to Pi 5
void readAndSendToFData() {
  // Move servo
  currentAngle += sweepDirection;
  if (currentAngle >= 160 || currentAngle <= 20) {
    sweepDirection = -sweepDirection; // Reverse sweep range (20° to 160°)
  }
  tofServo.write(currentAngle);

  // Measure distance when ready
  if (vl53.dataReady()) {
    int16_t distance = vl53.distance();
    vl53.clearInterrupt();

    if (distance > 0) {
      // Build JSON payload
      StaticJsonDocument<128> doc;
      doc["a"] = currentAngle;  // Angle in degrees
      doc["d"] = distance;      // Distance in millimeters

      char buffer[128];
      serializeJson(doc, buffer);

      // Transmit UDP packet to Raspberry Pi 5
      udp.beginPacket(PI5_IP_ADDRESS, UDP_TX_PORT);
      udp.write((const uint8_t*)buffer, strlen(buffer));
      udp.endPacket();
    }
  }
}

// Parses JSON motor commands from Pi 5: {"left": [-255 to 255], "right": [-255 to 255]}
void handleIncomingMotorCommands() {
  int packetSize = udp.parsePacket();
  if (packetSize > 0) {
    char packetBuffer[255];
    int len = udp.read(packetBuffer, 255);
    if (len > 0) {
      packetBuffer[len] = 0;
    }

    StaticJsonDocument<128> doc;
    DeserializationError error = deserializeJson(doc, packetBuffer);

    if (!error) {
      int left = doc["left"];   // Range: -255 to 255
      int right = doc["right"]; // Range: -255 to 255
      setMotorSpeeds(left, right);
    }
  }
}

// Motor Driver Control (Positive = Forward, Negative = Reverse)
void setMotorSpeeds(int leftSpeed, int rightSpeed) {
  // Left Motor (Motor A)
  if (leftSpeed >= 0) {
    digitalWrite(MOTOR_A_IN1, HIGH);
    digitalWrite(MOTOR_A_IN2, LOW);
    ledcWrite(CH_A, min(leftSpeed, 255));
  } else {
    digitalWrite(MOTOR_A_IN1, LOW);
    digitalWrite(MOTOR_A_IN2, HIGH);
    ledcWrite(CH_A, min(-leftSpeed, 255));
  }

  // Right Motor (Motor B)
  if (rightSpeed >= 0) {
    digitalWrite(MOTOR_B_IN1, HIGH);
    digitalWrite(MOTOR_B_IN2, LOW);
    ledcWrite(CH_B, min(rightSpeed, 255));
  } else {
    digitalWrite(MOTOR_B_IN1, LOW);
    digitalWrite(MOTOR_B_IN2, HIGH);
    ledcWrite(CH_B, min(-rightSpeed, 255));
  }
}
