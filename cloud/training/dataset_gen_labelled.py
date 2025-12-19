import random
import csv

# === BASE VALUES AT 10V ===
NORMAL_RANGES = {
    "c1": (1.0, 1.04),
    "c2": (0.96, 1.04),
    "c3": (0.96, 1.08),
}

THEFT_MAX = {
    "c1": 1.32,
    "c2": 1.36,
    "c3": 1.32,
}

JOINING_FACTOR_RANGES = {
    "normal": (1.10, 1.14),
    "blatant": (1.04, 1.08),
    "stealth": (1.07, 1.10),
}

# === SCALING ===
def scale_range(base_range, line_voltage):
    return tuple(round(v * (line_voltage / 10.0), 3) for v in base_range)

def scale_value(base_value, line_voltage):
    return round(base_value * (line_voltage / 10.0), 3)

# === FEEDER CALCULATION ===
def calculate_feeder(c1, c2, c3, target_jf):
    total = c1 + c2 + c3
    feeder = round(total / target_jf, 3)
    return feeder

# === SAMPLE GENERATOR ===
def generate_sample(line_voltage, mode):
    scaled_normal = {k: scale_range(v, line_voltage) for k, v in NORMAL_RANGES.items()}
    scaled_theft = {k: scale_value(v, line_voltage) for k, v in THEFT_MAX.items()}

    values = {}
    victim = None

    if mode == "normal":
        for k in ["c1", "c2", "c3"]:
            values[k] = round(random.uniform(*scaled_normal[k]), 3)

    elif mode in ["blatant", "stealth"]:
        victim = random.choice(["c1", "c2", "c3"])
        for k in ["c1", "c2", "c3"]:
            if k == victim:
                # Theft load is higher for blatant, subtler for stealth
                if mode == "blatant":
                    values[k] = round(random.uniform(scaled_normal[k][1] + 0.01, scaled_theft[k]), 3)
                else:  # stealth theft closer to normal upper limit
                    values[k] = round(random.uniform(scaled_normal[k][1] + 0.005, scaled_theft[k] * 0.95), 3)
            else:
                values[k] = round(random.uniform(*scaled_normal[k]), 3)

    target_jf = round(random.uniform(*JOINING_FACTOR_RANGES[mode]), 3)
    feeder = calculate_feeder(values["c1"], values["c2"], values["c3"], target_jf)

    label = "normal" if mode == "normal" else victim
    return [line_voltage, label, values["c1"], values["c2"], values["c3"], feeder]

# === DATASET GENERATION ===
def generate_dataset(filename="power_data_labeled.csv", n_samples=3000):
    rows = [["line_voltage", "label", "c1", "c2", "c3", "feeder"]]
    log_interval = 500

    for i in range(1, n_samples + 1):
        line_voltage = round(random.uniform(8.0, 12.0), 2)
        mode = random.choices(
            population=["normal", "blatant", "stealth"],
            weights=[0.6, 0.2, 0.2],
            k=1
        )[0]

        sample = generate_sample(line_voltage, mode)
        rows.append(sample)

        if i % log_interval == 0:
            print(f"[LOG] Generated {i}/{n_samples} samples...")

    with open(filename, mode="w", newline="") as f:
        writer = csv.writer(f)
        writer.writerows(rows)

    print(f"✅ Dataset generation complete: {filename}")

# === RUN ===
generate_dataset()
