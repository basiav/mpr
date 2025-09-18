#!/bin/bash

# --- Configuration ---
ALGO_NAME=$1
NUM_RUNS=5   # smaller number for local testing
BUCKETS=("1" "100" "1000" "5000" "6000" "7000" "8000" "9000" "10000" "20000" "35000" "50000" "100000")
OUTPUT_CSV="results.csv"
EXECUTABLE="./${ALGO_NAME}.exe"   # Windows build will be .exe
BUCKET_ID=3   # pick manually (instead of SLURM_ARRAY_TASK_ID)

# --- Input Validation ---
if [ -z "${ALGO_NAME}" ]; then
    echo "Error: No algorithm name provided."
    echo "Usage: $0 <algorithm_name>"
    exit 1
fi
if [ ! -f "${EXECUTABLE}" ]; then
    echo "Error: Executable ${EXECUTABLE} not found. Compile it first."
    exit 1
fi

# --- Create Log Directory ---
mkdir -p logs

# --- Set OpenMP Threads (only 1 for your test) ---
export OMP_NUM_THREADS=1

# --- Run the tests ---
TMP_RESULTS=$(mktemp)
for (( i=1; i<=NUM_RUNS; i++ ))
do
    buckets_count=${BUCKETS[$BUCKET_ID-1]}
    output_times=$(${EXECUTABLE} ${buckets_count})

    if [ $? -eq 0 ] && [ -n "$output_times" ]; then
        echo "${ALGO_NAME};${buckets_count};${output_times}" >> ${TMP_RESULTS}
    else
        echo "Warning: Run $i failed for ${ALGO_NAME} with ${buckets_count} buckets" >> "logs/run_error_${ALGO_NAME}_${buckets_count}.log"
    fi
done

cat ${TMP_RESULTS} >> ${OUTPUT_CSV}
rm ${TMP_RESULTS}

echo "Finished. Results appended to ${OUTPUT_CSV}"
