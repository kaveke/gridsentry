import json
import joblib
import numpy as np
import pandas as pd
import os

# Load model and scaler (only once)
model = joblib.load(os.path.join(os.path.dirname(__file__), "svm_power_theft_model.pkl"))
scaler = joblib.load(os.path.join(os.path.dirname(__file__), "scaler.pkl"))

# Feature columns
columns = ['line_voltage', 'c1', 'c2', 'c3', 'feeder']
label_map = {0: 'normal', 1: 'stealth_theft', 2: 'blatant_theft'}

def lambda_handler(event, context):
    try:
        # Get data from the incoming request (JSON body)
        data = event.get('body')
        if isinstance(data, str):
            data = json.loads(data)
        
        features = [data.get(col, 0.0) for col in columns]
        sample_df = pd.DataFrame([features], columns=columns)
        scaled_sample = scaler.transform(sample_df)
        prediction = model.predict(scaled_sample)

        return {
            'statusCode': 200,
            'body': json.dumps({
                'prediction': label_map[prediction[0]]
            })
        }
    except Exception as e:
        return {
            'statusCode': 500,
            'body': json.dumps({'error': str(e)})
        }
