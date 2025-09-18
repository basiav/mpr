#!/bin/bash

output_file="results.csv"
source_file="fill.c"
binary_file="fill.o"

# setup csv
echo "array_size;threads;scheduler;chunk_size;time" > ${output_file}

# function running the binary and saving the result to csv
run_test() {
    local current_size="$1"
    local current_scheduler_type="$2"
    local current_chunk_size="$3"
    local current_threads="$4"
    local iteration_count="$5"

    local scheduler_define="${current_scheduler_type}, ${current_chunk_size}"

    echo "Running: Size=$current_size, Scheduler=$current_scheduler_type, Chunk=$current_chunk_size, Threads=$current_threads, Iteration=$iteration_count"

    gcc "${source_file}" -o "${binary_file}" -fopenmp -DARRAY_SIZE=${current_size} -DSCHEDULER="${scheduler_define}"
    if [ $? -ne 0 ]; then
        echo "Error compiling for size=${current_size}, scheduler=${scheduler_define}" >&2
        return 1
    fi

    export OMP_NUM_THREADS="$current_threads"
    local time_output
    time_output=$(./"${binary_file}")
    if [ $? -ne 0 ]; then
        echo "Error running for size=${current_size}, scheduler=${scheduler_define}" >&2
        rm -f "${binary_file}"
        return 1
    fi

    echo "$current_size;$current_threads;${current_scheduler_type};${current_chunk_size};${time_output}" >> "${output_file}"
    rm -f "${binary_file}"
}

declare -A guided_chunk_sizes
declare -A static_chunk_sizes
repeat_testsuites=2
threads_counts=(1 2 3 4 5 6 7 8)
scheduler_types=("static" "dynamic" "guided")
sizes=(2000000 2000000000)
guided_chunk_sizes["2000000"]="1 10 200 20000"
guided_chunk_sizes["2000000000"]="1 10 200 20000 2000000"
static_chunk_sizes["2000000"]="1 100 10000 200000"
static_chunk_sizes["2000000000"]="1 100 10000 200000 200000000"

# perform tests
for i in $(seq 1 $repeat_testsuites); do
    for scheduler_type in "${scheduler_types[@]}"; do
        for threads in "${threads_counts[@]}"; do
            for size in "${sizes[@]}"; do
                chunk_list_string=""
                if [ "$scheduler_type" == "guided" ]; then
                    chunk_list_string="${guided_chunk_sizes["$size"]}"
                else
                    chunk_list_string="${static_chunk_sizes["$size"]}"
                fi

                current_chunk_sizes=()
                read -r -a current_chunk_sizes <<< "$chunk_list_string"

                for chunk_size in "${current_chunk_sizes[@]}"; do
                    run_test "$size" "$scheduler_type" "$chunk_size" "$threads" "$i"
                done
            done
        done
    done
done

