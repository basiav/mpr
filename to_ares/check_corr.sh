#!/bin/bash

JOB_ID=$1
SLURM_LOGS_DIR="${2:-slurm_logs}"

for f in "$SLURM_LOGS_DIR"/*_${JOB_ID}*.err; do echo "=== $f ==="; cat "$f"; done
