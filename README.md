# 🌦️ Weather Station Dashboard (MDP)

An end-to-end IoT Weather Monitoring System featuring a real-time React dashboard, ESP32 firmware with multiple sensors, and AI-driven weather insights powered by Groq (Llama 3.3).

## 📂 Project Structure

- **`/frontend`**: React + Vite + Tailwind CSS dashboard.
- **`/esp32_sensors`**: C++ firmware for ESP32 microcontroller.
- **`/backend`**: Firebase configuration and cloud functions (if applicable).

## 🚀 Key Features

- **Real-time Monitoring**: Instant updates for Temperature, Humidity, Pressure, and UV Index via Firebase RTDB.
- **Historical Data**: 20 most recent readings visualized with Recharts, stored in Firestore.
- **AI Weather Insights**: Structured analysis (Current Status & Future Prediction) using Groq API.
- **Live Local Weather**: Standalone localized weather reporter using Geolocation and Open-Meteo API.
- **Glassmorphism UI**: Modern, frosted-glass aesthetic with smooth animations.
- **Multi-Sensor ESP32**: Integrated DHT11, BMP180, and LTR390 sensors with dual-I2C bus support.

## 🛠️ Setup Instructions

### 1. ESP32 Firmware
- Open `esp32_sensors/esp32_sensors.ino` in Arduino IDE.
- Install required libraries: `Firebase ESP Client`, `DHT sensor library`, `Adafruit BMP085`, `Adafruit LTR390`.
- Update WiFi credentials and Firebase API key in the code.
- Upload to your ESP32 board.

### 2. Frontend Dashboard
- Navigate to `cd frontend`.
- Install dependencies: `npm install`.
- Create a `.env` file with your Firebase and Groq API credentials.
- Run locally: `npm run dev`.

### 3. Firebase Configuration
- Enable **Anonymous Authentication** in the Firebase Console.
- Set up **Realtime Database** and **Firestore** with appropriate security rules.

## ⚖️ License
MIT License
