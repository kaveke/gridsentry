import json
import numpy as np
import joblib

# Load model and scaler (assumes they're in the same directory as this script in the Lambda package)
model = joblib.load('svm_power_theft_model.pkl')
scaler = joblib.load('scaler.pkl')

# Label mapping for predictions
label_map = {
    0: 'normal',
    1: 'stealth_theft',
    2: 'blatant_theft'
}

def lambda_handler(event, context):
    try:
        # Parse input from event
        body = json.loads(event['body']) if 'body' in event else event
        sample = body.get("sample")

        if not sample or len(sample) != 5:
            return {
                "statusCode": 400,
                "body": json.dumps({"error": "Sample must be a list of 5 numeric values."})
            }

        # Preprocess the sample
        sample_array = np.array([sample])
        sample_scaled = scaler.transform(sample_array)

        # Make prediction
        prediction = model.predict(sample_scaled)
        label = label_map[prediction[0]]

        return {
            "statusCode": 200,
            "body": json.dumps({
                "input": sample,
                "prediction": label
            })
        }

    except Exception as e:
        return {
            "statusCode": 500,
            "body": json.dumps({"error": str(e)})
        }
