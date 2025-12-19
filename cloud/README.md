# GridSentry Cloud and Intelligence Infrastructure

This directory contains the serverless backend logic, machine learning pipeline, and database integration for the GridSentry system. The architecture leverages AWS Lambda, Timestream, and Dockerized inference to provide real-time theft detection and historical analytics.

## 1. Architecture Overview

The cloud backend processes data via two parallel streams:

1.  **The "Hot" Path (Inference):** Incoming sensor data is immediately routed to a Machine Learning model to detect theft. If theft is detected, a command is sent back to the device within milliseconds.
2.  **The "Cold" Path (Storage):** The same data is written to a Time-Series database for long-term storage and visualization in Grafana.

## 2. Serverless Components (Lambda)

### A. The Router: router_lambda

* **Role:** Traffic Controller and Data Pre-processor.
* **Trigger:** AWS IoT Rule (Topic: smartmeter/data).
* **Functionality:**
    * Receives raw JSON from the ESP32.
    * Extracts and sanitizes voltage/current readings (handles type conversions).
    * Formats the payload specifically for the SVM Model.
    * Invokes the **Inference Lambda** synchronously.
    * Publishes the result back to the device on topic `smartmeter/prediction`.

### B. The Storage Handler: smartmeter-to-timestream

* **Role:** Data Persistence.
* **Trigger:** AWS IoT Rule (Topic: smartmeter/data).
* **Functionality:**
    * Parses the ISO 8601 timestamp (%Y-%m-%dT%H:%M:%S%z).
    * Calculates Power (P = V * I) for the Feeder and Transformers.
    * Writes records to **AWS Timestream** with dimensions `source=feeder` or `source=transformerX`.

### C. The ML Brain: PowerTheftDetector_model

* **Role:** Anomaly Detection.
* **Trigger:** Invoked directly by router_lambda.
* **Deployment:** Docker Container (AWS ECR).
* **Functionality:**
    * Loads the pre-trained `theft_detector_svm.pkl`.
    * Scales input data using `scaler.pkl`.
    * Returns a classification: ["Normal"] or ["Theft"].

## 3. Machine Learning Pipeline

The ML components are located in `cloud/training` and `cloud/models`.

1.  **Dataset Generation:** `dataset_gen.py` creates synthetic power signatures simulating bypass attacks and line tampering.
2.  **Training:** `SVM_power_theft.ipynb` trains a Support Vector Machine (SVM) classifier.
3.  **Serialization:** The trained model and scalers are exported as .pkl files to `cloud/models`.
4.  **Containerization:** These models are packaged into a Docker image to ensure scikit-learn dependencies are consistent between training and inference.

## 4. Deployment Instructions

### Step 1: Deploy the ML Model (Docker)

Because the ML libraries are large, we use Docker.

1.  Navigate to `cloud/lambda_inference`.
2.  Build the image:
    ```bash
    docker build -t grid-sentry-inference .
    ```
3.  Push to **Amazon ECR** and create a Lambda function from the Container Image named `PowerTheftDetector_model`.

### Step 2: Create AWS IoT Rules

You need two rules in AWS IoT Core > Message Routing > Rules.

**Rule 1: Inference Routing**

* **SQL Statement:** `SELECT * FROM 'smartmeter/data'`
* **Action:** Send a message to a Lambda function (`router_lambda`).

**Rule 2: Historical Storage**

* **SQL Statement:** `SELECT * FROM 'smartmeter/data'`
* **Action:** Send a message to a Lambda function (`smartmeter-to-timestream`).

### Step 3: Configure Grafana

To visualize the data stored in Timestream:

1.  Install the **Amazon Timestream** plugin in Grafana.
2.  Configure the data source with your AWS credentials and region (us-east-1).
3.  Select Database: `SmartMeterDB` and Table: `SmartMeterReadings`.
4.  Use the following sample query for voltage monitoring:

    ```sql
    SELECT time, measure_value::double as Voltage
    FROM "SmartMeterDB"."SmartMeterReadings"
    WHERE measure_name = 'line_voltage'
    ORDER BY time DESC
    ```

## 5. External Documentation References

* **AWS IoT Rules:** Creating and Managing Rules
* **Timestream and Grafana:** Visualizing Timestream data with Grafana
* **Lambda with Containers:** Deploying Lambda functions with container images

## Action Items for your Instructions/ Folder

You should create a text file or markdown file there (e.g., Instructions/Cloud_Setup_Checklist.md) with the following checklist to ensure the system is interconnected correctly:

* **Create Timestream Database:**
    * Database Name: `SmartMeterDB`
    * Table Name: `SmartMeterReadings`
    * **Note:** Ensure the memory store retention is set to at least 24 hours.

* **IAM Roles:**
    * Ensure `router_lambda` has permission: `lambda:InvokeFunction` (to call the model) and `iot:Publish` (to send alerts back to the ESP32).
    * Ensure `smartmeter-to-timestream` has permission: `timestream:WriteRecords`.

* **Docker Build:**
    * Ensure `cloud/lambda_inference/Dockerfile` copies the contents of `cloud/models/` into the image. If the model files are missing, the Lambda will fail to load the SVM.

* **Grafana Alerting (Optional):**
    * Set up a Grafana alert if `line_voltage` drops below 200V, providing a secondary visual alert system for the operator.