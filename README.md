# IoT Smart Car with Cloud Telemetry (Arduino UNO R4)

![Hardware](https://img.shields.io/badge/Hardware-Arduino%20UNO%20R4%20WiFi-blue.svg)
![IoT](https://img.shields.io/badge/Cloud-ThingSpeak%20API-green.svg)
![Sensors](https://img.shields.io/badge/Sensors-Ultrasonic%20%7C%20BH1750-orange.svg)

## 📖 Introduction

This project is a multi-functional IoT Smart Car designed to demonstrate sensor fusion and cloud connectivity. Unlike traditional RC cars, this vehicle acts as a mobile environmental station. It allows for Bluetooth remote control while simultaneously monitoring its surroundings (distance and light levels) and uploading telemetry data to the **ThingSpeak** cloud platform in real-time.

The firmware is optimized using non-blocking logic (`millis()`) to ensure smooth motor control while handling network requests and LED animations concurrently.

### ✨ Key Features

* **Remote Control:** Bluetooth control via HC-05 using a mobile app (supports omnidirectional movement).
* **IoT Data Logging:** Uses the on-board Wi-Fi of the Arduino UNO R4 to upload ambient brightness (Lux) data to ThingSpeak every 20 seconds.
* **Automatic Safety System:** Ultrasonic sensor continuously monitors distance; triggers a buzzer alarm if obstacles are within 10cm.
* **Adaptive Lighting:**
    * **Front Lights:** Toggleable via Bluetooth.
    * **Rear Lights (WS2812B):** Automatically activates rainbow animations when ambient light is low (< 5 Lux).

---

## 🛠️ Hardware & BOM

The system is built around the **Arduino UNO R4 WiFi**, which combines the processing power of the RA4M1 chip with the connectivity of the ESP32-S3.

| Component | Type/Model | Function |
| :--- | :--- | :--- |
| **Main Controller** | Arduino UNO R4 WiFi | Central processing & Wi-Fi connectivity |
| **Motor Driver** | L298N | Drives 4x DC Motors (High current) |
| **Motors** | 4x TT Gear Motors | Propulsion |
| **Wireless Control** | HC-05 Bluetooth | Serial communication with mobile app |
| **Distance Sensor** | HC-SR04 | Obstacle detection (Ultrasonic) |
| **Light Sensor** | BH1750 (I2C) | Ambient light measurement (Lux) |
| **Smart LEDs** | WS2812B Strip | Rear signal lights (Addressable RGB) |
| **Power** | 2x 18650 Li-ion | 7.4V Power supply |
| **Others** | Buzzer, LEDs, Resistors | Signaling and indicators |

---

## 🔌 Circuit & Pinout

The following wiring diagram illustrates the connections between the sensors, drivers, and the microcontroller.

<img width="5180" height="2616" alt="ECC3313_Car_bb2" src="https://github.com/user-attachments/assets/17195674-fba8-43ed-a4fa-2d8a80e99c74" />


### Pin Configuration

| Component | Arduino Pin | Notes |
| :--- | :--- | :--- |
| **Motors (L298N)** | D5, D6, D10, D11 | PWM Control |
| **Ultrasonic Trig** | D12 | |
| **Ultrasonic Echo** | D13 | |
| **Bluetooth RX/TX** | Serial1 (D0/D1) | Hardware Serial on R4 |
| **WS2812B LED** | A2 | Data Line |
| **Buzzer** | A1 | |
| **Front LEDs** | A0, D9, A3 | |
| **BH1750** | SDA / SCL | I2C Protocol |

---

## 💻 Software Architecture

The firmware is written in C++ and utilizes a **Loop-based State Machine** to handle multiple tasks without pausing the processor.

### Core Logic Flow
1.  **Motor Control:** Reads `Serial1` buffer for Bluetooth commands (`F`, `B`, `L`, `R`, etc.).
2.  **Sensor Polling:**
    * **Ultrasonic:** Checks distance every **1 second**.
    * **Light Sensor:** Reads Lux every **2 seconds**.
3.  **Automation Rules:**
    * `IF distance <= 10cm` -> **Trigger Buzzer**.
    * `IF Lux < 5` -> **Activate WS2812B Animation**.
4.  **Cloud Upload:**
    * Every **20 seconds**, the system initiates an HTTP GET request to the ThingSpeak API to log the current brightness level.

---

## 📊 IoT Visualization (ThingSpeak)

The car uploads data to a public ThingSpeak channel. Below is an example of the data visualization pipeline:

1.  **Collection:** BH1750 measures light intensity.
2.  **Transmission:** Arduino connects to Wi-Fi and hits the API endpoint.
3.  **Visualization:** ThingSpeak generates real-time charts for environmental monitoring.

*(You can add a screenshot of your ThingSpeak chart here if available)*

---

## 🚀 How to Run

1.  **Library Installation:**
    Install the following libraries via Arduino IDE Library Manager:
    * `Adafruit NeoPixel`
    * `BH1750`
    * `ArduinoHttpClient` (Official)
    * `WiFiS3` (For UNO R4)
2.  **Configuration:**
    Open the `.ino` file and update your credentials:
    ```cpp
    const char ssid[] = "YOUR_WIFI_SSID";
    const char pass[] = "YOUR_WIFI_PASSWORD";
    const char writeAPIKey[] = "YOUR_THINGSPEAK_API_KEY";
    ```
3.  **Upload:**
    Connect the Arduino UNO R4 and upload the code.
4.  **Control:**
    Use a standard "Bluetooth RC Car" app (sending characters 'F', 'B', 'L', 'R') to control the vehicle.

---

## 📜 License

This project is open-source and available under the [MIT License](LICENSE).
