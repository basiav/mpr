#!/bin/bash
#SBATCH --job-name=sort_test_%A_%a
#SBATCH --account=plgmpr25-cpu
#SBATCH --partition=plgrid
#SBATCH --array=1-16
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=16
#SBATCH --output=slurm_logs/slurm_%A_%a.out
#SBATCH --error=slurm_logs/slurm_%A_%a.err
#SBATCH --time=02:00:00
#SBATCH --mem-per-cpu=500M

# --- Configuration ---
ALGO_NAME=$1
NUM_RUNS=20
OUTPUT_CSV="results.csv"
EXECUTABLE="./${ALGO_NAME}.o"
THREADS=${SLURM_ARRAY_TASK_ID}

# --- Input Validation ---
if [ -z "${ALGO_NAME}" ]; then
    echo "Error: No algorithm name provided."
    echo "Usage: sbatch $0 <algorithm_name>"
    exit 1
fi
if [ ! -f "${EXECUTABLE}" ]; then
    echo "Error: Executable ${EXECUTABLE} not found. Compile it first."
    exit 1
fi

# --- Create Log Directory ---
mkdir -p slurm_logs

# --- Set OpenMP Threads ---
export OMP_NUM_THREADS=${THREADS}

# --- Run the tests ---
TMP_RESULTS=$(mktemp)
for (( i=1; i<=NUM_RUNS; i++ ))
do
    # Execute the program and capture output
    output_times=$(${EXECUTABLE})

    # Check if execution was successful and produced output
    if [ $? -eq 0 ] && [ -n "$output_times" ]; then
        echo "${ALGO_NAME};${THREADS};${output_times}" >> ${TMP_RESULTS}
    else
        echo "Warning: Run $i for ${ALGO_NAME} with ${THREADS} threads failed or produced no output." >> "slurm_logs/run_error_${ALGO_NAME}_${THREADS}.log"
    fi
done

# --- Append results to the main CSV file ---
cat ${TMP_RESULTS} >> ${OUTPUT_CSV}
rm ${TMP_RESULTS}

exit 0