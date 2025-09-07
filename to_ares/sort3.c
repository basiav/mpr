#include "buckets.h"
#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

#define ARRAY_SIZE 20000000
#define BUCKETS 8000
#define BUCKET_SIZE_OVERHEAD 2

typedef int array_element_t;
typedef array_element_t* array_t;

int main() {
    array_t array = malloc(sizeof(array_element_t) * ARRAY_SIZE);
    if (!array) { perror("Array alloc failed"); return 1; }

    int num_threads = omp_get_max_threads();

    size_t buckets_per_thread = BUCKETS;

    // Allocate thread buckets array
    Bucket_t*** thread_buckets = malloc(num_threads * sizeof(Bucket_t**));
    for (int t = 0; t < num_threads; t++) {
        thread_buckets[t] = malloc(buckets_per_thread * sizeof(Bucket_t*));
    }

    // Each thread fills its own buckets
    #pragma omp parallel
    {
        int tid = omp_get_thread_num();
        unsigned int seed = tid;

        // Allocate and initialize per-thread buckets
        size_t init_bucket_size = (ARRAY_SIZE / num_threads / buckets_per_thread) * BUCKET_SIZE_OVERHEAD;
        for (size_t b = 0; b < buckets_per_thread; b++) {
            thread_buckets[tid][b] = malloc(sizeof(Bucket_t));
            initialize_bucket(thread_buckets[tid][b], init_bucket_size);
        }

        // Fill array in parallel
        #pragma omp for
        for (size_t i = 0; i < ARRAY_SIZE; i++)
            array[i] = rand_r(&seed);

        // Distribute values to buckets
        #pragma omp for
        for (size_t i = 0; i < ARRAY_SIZE; i++) {
            size_t b = array[i] * buckets_per_thread / ((unsigned long long)RAND_MAX + 1);
            add_element_to_bucket(thread_buckets[tid][b], array[i]);
        }
    }

    // Merge buckets: each bucket index is merged across threads
    Bucket_t* merged_buckets = malloc(buckets_per_thread * sizeof(Bucket_t));
    for (size_t b = 0; b < buckets_per_thread; b++) {
        size_t total_count = 0;
        for (int t = 0; t < num_threads; t++)
            total_count += thread_buckets[t][b]->count;
        initialize_bucket(&merged_buckets[b], total_count);

        // Copy all elements from threads into merged bucket
        size_t offset = 0;
        for (int t = 0; t < num_threads; t++) {
            Bucket_t* src = thread_buckets[t][b];
            for (size_t j = 0; j < src->count; j++)
                merged_buckets[b].elements[offset++] = src->elements[j];
        }
        merged_buckets[b].count = total_count;
    }

    // Sort each merged bucket
    #pragma omp parallel for
    for (size_t b = 0; b < buckets_per_thread; b++)
        qsort(merged_buckets[b].elements, merged_buckets[b].count, sizeof(int), compare_function);

    // Copy back to the original array
    size_t offset = 0;
    for (size_t b = 0; b < buckets_per_thread; b++) {
        for (size_t j = 0; j < merged_buckets[b].count; j++)
            array[offset++] = merged_buckets[b].elements[j];
    }

    // Free memory
    for (int t = 0; t < num_threads; t++) {
        for (size_t b = 0; b < buckets_per_thread; b++) {
            free_bucket_elements(thread_buckets[t][b]);
            free(thread_buckets[t][b]);
        }
        free(thread_buckets[t]);
    }
    free(thread_buckets);

    for (size_t b = 0; b < buckets_per_thread; b++) {
        free_bucket_elements(&merged_buckets[b]);
    }
    free(merged_buckets);
    free(array);

    return 0;
}
