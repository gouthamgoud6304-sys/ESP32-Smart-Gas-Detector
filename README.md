# 🚨 ESP32 Smart Gas Detector (IoT Safety System)

## 📌 Project Overview
A professional-grade IoT gas leak detection system featuring real-time monitoring, multi-channel emergency alerts and a custom configuration portal. This project is designed with a focus on system stability and reliable communication in critical safety environments.

**Firmware Version:** `GD-080126-V3`

## 🚀 Key Features
* **Dual-Core Processing:** Utilizes **FreeRTOS** to handle sensor sampling and network tasks on separate ESP32 cores for maximum uptime.
* **Multi-Channel Alerts:** Instant notifications via **WhatsApp** (CallMeBot), **Telegram Bot** and **HTML Email** (Gmail SMTP).
* **Captive Portal:** On-device web server (WiFiManager) for configuring WiFi, alert thresholds and contact details without reflashing code.
* **OLED Dashboard:** 128x64 display showing real-time gas levels, NTP-synced time and system health.
* **Fail-Safe Logic:** Features WiFi rollback logic and sensor preheating routines to prevent false alarms.

## 🛠️ Hardware Requirements
* **Microcontroller:** ESP32 (DevKit V1)
* **Sensor:** MQ-Series Gas Sensor (MQ-5)
* **Display:** SSD1306 OLED (I2C)
* **Indicators:** Active Buzzer & LED Alert
* **User Input:** Config Button (Portal trigger) & Mute Button (Interrupt-driven)



## 📂 Project Structure
* `Smart_Gas_Detector.ino`: Main firmware containing logic for FreeRTOS tasks and alert systems.
* `config.json`: (Stored via SPIFFS) Keeps user settings persistent across power cycles.
* `.gitignore`: Prevents temporary build files from cluttering the repository.

## 🧪 Engineering & Validation Focus
As an **Embedded Testing Engineer**, this project emphasizes:
1.  **Interrupt Handling:** Using `IRAM_ATTR` for the buzzer mute button to ensure immediate response.
2.  **Task Management:** Critical section management (`portENTER_CRITICAL`) for data sharing between cores.
3.  **Persistence:** JSON-based configuration management using **SPIFFS** for reliable data storage.
4.  **Error Handling:** Implementation of watchdog-like behavior for WiFi reconnection attempts.

## 📜 License
This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
