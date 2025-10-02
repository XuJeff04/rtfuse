import pandas as pd

# Load the results
df = pd.read_csv("results.csv")

# Compute mean + std grouped by system, iteration, and op
grouped = df.groupby(["system", "iterations", "op"]).agg(
    rwtest_time_ms_mean=("rwtest_time_ms", "mean"),
    rwtest_time_ms_std=("rwtest_time_ms", "std"),
    real_time_s_mean=("real_time_s", "mean"),
    real_time_s_std=("real_time_s", "std")
).reset_index()

# Round to 2 decimal places
grouped = grouped.round(2)

# Separate into four datasets, renaming consistently
write_rwtest = grouped[grouped["op"] == "write"][["system", "iterations", "rwtest_time_ms_mean", "rwtest_time_ms_std"]] \
    .rename(columns={"rwtest_time_ms_mean": "time", "rwtest_time_ms_std": "stdv"})

write_real = grouped[grouped["op"] == "write"][["system", "iterations", "real_time_s_mean", "real_time_s_std"]] \
    .rename(columns={"real_time_s_mean": "time", "real_time_s_std": "stdv"})

read_rwtest = grouped[grouped["op"] == "read"][["system", "iterations", "rwtest_time_ms_mean", "rwtest_time_ms_std"]] \
    .rename(columns={"rwtest_time_ms_mean": "time", "rwtest_time_ms_std": "stdv"})

read_real = grouped[grouped["op"] == "read"][["system", "iterations", "real_time_s_mean", "real_time_s_std"]] \
    .rename(columns={"real_time_s_mean": "time", "real_time_s_std": "stdv"})

# Save to CSV
write_rwtest.to_csv("summary_write_rwtest.csv", index=False)
write_real.to_csv("summary_write_real.csv", index=False)
read_rwtest.to_csv("summary_read_rwtest.csv", index=False)
read_real.to_csv("summary_read_real.csv", index=False)

# Save full grouped results
grouped.to_pickle("summary_grouped.pkl")
