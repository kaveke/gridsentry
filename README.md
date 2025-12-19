# 🔋 GridSentry: AI-Powered Energy Theft Detection System

**GridSentry** is an end-to-end IoT solution designed to detect electricity theft (meter bypassing and tampering) in real-time. It leverages an ESP32-based smart meter for edge telemetry and a serverless AWS Cloud backend for Machine Learning inference.

![Architecture](docs/assets/Architecture.jpg)

---

## 📁 Repository Structure

This repository is organized into three main modules. Please refer to the specific README in each directory for detailed setup instructions.

| Module | Description |
| :--- | :--- |
| **[Firmware](firmware_files/README.md)** | ESP32 embedded code (ESP-IDF). Handles sensor acquisition (INA219/3221), mTLS security, and MQTT transmission. |
| **[Cloud & AI](cloud/README.md)** | AWS Lambda functions, Dockerized SVM inference engine, and Timestream/Grafana integration. |
| **[Hardware](hardware/)** | Schematics, PCB designs, and Bill of Materials (BOM). |

---

## 🚀 Quick Start Guide

### 1. Clone the Repository
```bash
git clone https://github.com/kaveke/gridsentry.git
cd gridsentry
```

### 2. Hardware Setup
Assemble the PCB according to the [Hardware Schematics](hardware/schematics/).

### 3. Cloud Deployment
Deploy the Dockerized Lambda model and IoT rules as described in the [Cloud Documentation](cloud/README.md).

### 4. Firmware Configuration
Flash the ESP32 using the guide in [Firmware Documentation](firmware_files/README.md).

---

## 🧠 Data Science & Model Training

Interested in how the theft detection model works? 

I have provided the complete dataset generation and training pipeline in the `cloud/training/` directory. You can use these resources to:

- **Generate new datasets:** Use `dataset_gen_labelled.py` to simulate different theft scenarios (e.g., varying impedance bypasses).
- **Retrain the Model:** Open `Identifier_model.ipynb` in Jupyter to retrain the Support Vector Machine with new parameters or data.
- **Test Architectures:** Swap the SVM for Random Forest or Neural Networks by modifying the training notebook and exporting new `.pkl` files.

---

## 📦 Third-Party Dependencies

This project includes the following third-party components as static dependencies:

### ESP-AWS-IoT SDK
- **Source:** [espressif/esp-aws-iot](https://github.com/espressif/esp-aws-iot)
- **License:** MIT
- **Location:** `firmware_files/components/esp-aws-iot/`
- **Purpose:** AWS IoT Core connectivity for ESP32 devices

The ESP-AWS-IoT SDK is included directly in this repository for ease of deployment. All credit for this library goes to Espressif Systems and AWS.

---

## 🙏 Attribution

- **ESP-AWS-IoT SDK:** [Espressif Systems](https://github.com/espressif/esp-aws-iot) - AWS IoT integration layer
- **Core Embedded Logic:** Adapted from state machine patterns by **Kevin Aguilar**
- **I2C Drivers:** Based on libraries by **Ruslan V. Uss**
- **Project Maintainer:** [Kaveke](https://github.com/kaveke)

---

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

**Note:** Third-party components (ESP-AWS-IoT SDK) retain their original licenses.

---

<div align="center">

**🔋 Built by [Kaveke](https://github.com/kaveke)**

*Demonstrating full-stack IoT development: embedded systems, cloud ML, and hardware design*

</div>
