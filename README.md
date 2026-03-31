# PedalGuard - IoT Bike Anti-Theft Device

PedalGuard is a comprehensive IoT-based anti-theft system designed to protect bicycles and small vehicles. It combines hardware sensors (vibration and GPS) with a Node/Express backend and a responsive web application to offer real-time monitoring, alerts, and location tracking.

## System Architecture

The project consists of three main components:

1. **IoT Device (Hardware)**
   - **Microcontroller**: NodeMCU 1.0 (ESP8266)
   - **Sensors**: SW-420 Vibration Sensor (for movement detection), NEO-6M GPS Module (for location tracking)
   - **Actuators**: Piezo Buzzer (local alarm), Red/Green LEDs (status indicators)
   - **Behavior**: The device continuously polls the server for its armed/disarmed status. When armed, if vibration is detected, it triggers a local alarm sequence and sends an alert payload (including GPS coordinates) to the Node.js backend.

2. **Backend Server (`server.js`)**
   - **Framework**: Node.js & Express.js
   - **Function**: Acts as the bridge between the IoT device and the Firebase database.
   - **Endpoints**:
     - `GET /api/getstatus`: Returns the current system status (ARMED or DISARMED) to the device.
     - `POST /api/setstatus`: Allows the web application to arm or disarm the system.
     - `POST /api/alert`: Receives theft alerts from the device, appends a server timestamp, and pushes the data to Firebase Realtime Database.

3. **Web Application (Frontend)**
   - **Interface**: Built with HTML, Tailwind CSS, and vanilla JavaScript.
   - **Authentication**: Fully integrated with Firebase Authentication (Sign up and Login).
   - **Real-time Monitoring**: Connects directly to Firebase Realtime Database to display live alerts and notifications using `global-notifications.js`.
   - **Location Tracking**: Uses Leaflet.js (`loc.html`) to display the live GPS location of the bike based on the latest alerts.
   - **Feedback System**: Users can submit feedback directly to Firebase Firestore (`Feedback.html`).

## Features

- **Remote Arm/Disarm**: Control the security state of your bike from anywhere using the web dashboard.
- **Local Alarm System**: The IoT device triggers a loud buzzer and flashing LED pattern to deter thieves.
- **Real-Time Push Notifications**: Instant global toast notifications on the web app when a theft attempt occurs.
- **Live GPS Tracking**: Real-time location mapping of the alerts using Leaflet mapping.
- **Alert History**: View a detailed log of past alert events (`alert.html`).
- **Secure Authentication**: Firebase-powered user accounts.

## Project Structure

```text
PedalGuard/
├── arduino codes/
│   └── sketch_oct6a.ino       # ESP8266 C++ source code for the IoT device
├── server.js                  # Node.js backend server
├── package.json               # Node.js dependencies
├── Home.html                  # Main dashboard page
├── login.html                 # User login page
├── signup.html                # User registration page
├── alert.html                 # Alert history and management
├── loc.html                   # Live GPS location map
├── Feedback.html              # User feedback form
├── aboutus.html               # About the project
└── global-notifications.js    # Background notification handler
```

## Setup & Installation

### 1. Web Application & Firebase Setup
1. Create a [Firebase Project](https://console.firebase.google.com/).
2. Enable **Authentication** (Email/Password), **Realtime Database**, and **Firestore**.
3. Obtain your Firebase configuration details (API Key, Auth Domain, Project ID, Database URL, etc.).
4. Update the template variables (`YOUR_API_KEY`, `YOUR_DATABASE_URL`, etc.) in all `.html` and `.js` frontend files.

### 2. Backend Server Setup
1. Make sure Node.js is installed.
2. In the project directory, install dependencies:
   ```bash
   npm install
   ```
3. Go to your Firebase project settings, generate a new Service Account private key, and save it as `firebase-key.json` in the project root.
4. Open `server.js` and replace `'YOUR_DATABASE_URL'` with your actual Firebase Realtime Database URL.
5. Start the server:
   ```bash
   node server.js
   ```

### 3. IoT Device Setup
1. Open up `arduino codes/sketch_oct6a.ino` in the Arduino IDE.
2. Ensure you have installed the **ESP8266** board definitions and all required libraries (`ArduinoJson`, `TinyGPS++`).
3. Update the `ssid` and `password` variables to match your local Wi-Fi network.
4. Update `getStatusUrl` and `sendAlertUrl` with the correct IP address and port of your Node.js backend server.
5. Flash the code to the NodeMCU board.

## Usage
Once everything is running, visit the web application, log in, and use the dashboard to set the status to **ARMED**. The IoT device will fetch the updated status. If the vibration sensor is triggered, the system will execute an alarm sequence, log the alert to the backend, and push real-time notifications to the web dashboard along with live GPS coordinates.
