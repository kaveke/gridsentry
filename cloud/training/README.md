# Machine Learning Playground

This directory contains the tools used to create the "Brain" of GridSentry.

## Files Overview

* **`dataset_gen.py`**: A Python script that simulates electrical physics. It generates "Normal" and "Theft" samples by mathematically modeling voltage sags and current bypasses. Run this to create `power_data_labeled.csv`.
* **`SVM_power_theft.ipynb`**: The Jupyter Notebook used to:
    1.  Load the CSV data.
    2.  Normalize features using a Standard Scaler.
    3.  Train the Support Vector Machine (SVM).
    4.  Evaluate accuracy (Confusion Matrix).
    5.  Export the model (`.pkl`) for the AWS Cloud.

## How to Experiment
1.  Modify `dataset_gen.py` to add noise or new theft patterns.
2.  Run the generator to create a new CSV.
3.  Open the Notebook and retrain the model.
4.  Copy the resulting `.pkl` files to `../models/` and redeploy your Docker container to test your new model in production.