import json
import joblib
import numpy as np

# Load model and scaler once (reuse across invocations)
model = joblib.load("theft_detector_svm.pkl")
scaler = joblib.load("scaler.pkl")

# Class index mapping (adjust as per your model’s training)
label_map_index = {
    0: "normal",
    1: "c1",
    2: "c2",
    3: "c3"
}

def lambda_handler(event, context=None):
    try:
        # Extract features from input
        features = np.array([[event['line_voltage'], event['c1'], event['c2'], event['c3'], event['feeder']]])

        # Scale input
        features_scaled = scaler.transform(features)

        # Get prediction
        pred = model.predict(features_scaled)[0]
        pred_label = label_map_index.get(pred, "unknown")

        return {
            "statusCode": 200,
            "body": json.dumps({"prediction": [pred_label]})
        }

    except Exception as e:
        return {
            "statusCode": 500,
            "body": json.dumps({"error": str(e)})
        }
