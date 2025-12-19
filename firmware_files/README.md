This directory contains the embedded source code for GridSentry. It is built on the **Espressif IoT Development Framework (ESP-IDF v5.x)** and utilizes FreeRTOS for task management. The firmware is responsible for high-precision sensor acquisition, secure mTLS authentication, and reliable MQTT telemetry transmission to AWS IoT Core.

## 1. System Architecture

The firmware operates on a multi-threaded architecture to ensure non-blocking operation:

* **Sensor Task:** Polls INA219 (Feeder) and INA3221 (Transformer) sensors via I2C.
* **Network Task:** Manages WiFi provisioning and AWS IoT Core connection state.
* **Time Sync:** Uses SNTP to synchronize with global time servers for accurate timestamping of theft events.
* **Alert System:** Triggers physical GPIO responses (LEDs/Buzzers) upon receiving theft prediction payloads from the cloud.

#### Key Features

* **Secure Provisioning:** Uses Non-Volatile Storage (NVS) for WiFi credentials, avoiding hardcoded secrets.
* **OTA Updates:** Supports Over-The-Air firmware updates using a custom two-slot partition scheme (partitions_two_ota.csv).
* **Embedded Web Dashboard:** A lightweight HTML/CSS/JS interface hosted directly on the ESP32 for local configuration and status monitoring.
* **AI Integration:** Real-time theft detection via a machine learning inference engine hosted in the cloud.

### 2. Repository Structure

This project uses a clean monorepo structure, separating the firmware, cloud intelligence, and hardware documentation.

* **firmware/**: The complete ESP-IDF project source code, header files, and build configuration.
* **cloud/**: Python source code, Jupyter training notebooks (.ipynb), and the SVM model binary (.pkl).
* **hardware/**: Schematics, Bill of Materials (BOM), and wiring diagrams.
* **docs/**: Visual assets, whitepapers, and project diagrams.

### 3. Installation and Build Guide

#### A. Initialization (The Clean Start)

The most reliable method to prepare the project environment is by using the VS Code extension's New Project Wizard to establish the correct build paths.

1.  **Launch Wizard:** Open VS Code, press Ctrl+Shift+P, and select **ESP-IDF: New Project Wizard**.
2.  **Select Template:** Choose the **hello_world** template.
3.  **Set Directory:** Set the Project Directory to the root of your cloned repository (e.g., .../gridsentry/firmware).
4.  **Transfer Files:** Overwrite the placeholder hello_world files with the project's source files (main/*.c, main/*.h, components/, etc.).

#### C. Configuration and OpSec (Critical)

**Security Notice:** This project uses the ESP-IDF Kconfig system to manage sensitive credentials. Do not hardcode passwords in source files.

You must configure the device before building:

1.  **Open Configuration Menu:**
    Run the following command in the terminal (or use the VS Code extension):
    ```bash
    idf.py menuconfig
    ```

2.  **Navigate to "GridSentry Configuration":**
    In the top-level menu, select GridSentry Configuration. Set the following parameters:
    * **WiFi SSID:** The name of the Access Point the device will connect to/create.
    * **WiFi Password:** The WPA2 password.
    * **Local Timezone:** Set your region's timezone in POSIX format (e.g., EAT-3 for East Africa).
    * **AWS Client ID:** Set a unique ID for this device (e.g., GridSentry_001). **Note:** Duplicate IDs will cause AWS IoT Core to disconnect devices.

3.  **Configure Memory and Partitions:**
    While still in menuconfig, align the memory map:
    * **Serial Flasher Config:** Set Flash size to 4MB.
    * **Partition Table:** Select **Custom partition table CSV file**.
    * **Custom partition CSV file:** Set to `partitions_two_ota.csv`.

#### D. Build and Flash

Once configured, save the settings (press S, then Esc) and run:

    ```bash
    idf.py build flash monitor
    ```
