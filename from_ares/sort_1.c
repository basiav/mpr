#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "omp.h"
#include "buckets.h"

#define MEASURE_TIME(x) \
_Pragma("omp master")   \
x = omp_get_wtime();

#define ARRAY_SIZE 20000000
#define BUCKETS_PER_THREAD 100000
#define BUCKET_SIZE_OVERHEAD 4

typedef int  array_element_t;
typedef array_element_t* array_t;


int array_is_sorted(array_t array) {
    for(size_t i = 0; i < ARRAY_SIZE - 1; i++) {
        if(array[i] > array[i+1]) {
            fprintf(stderr, "Błąd: Tablica nie jest posortowana! array[%zu]=%d > array[%zu]=%d\n", i, array[i], i+1, array[i+1]);
            return 0;
        }
    }
    return 1;
}

int compare_function(const void *a, const void *b) {
    int *x = (int *) a;
    int *y = (int *) b;
    return *x - *y;
}

int main(int argc, char** argv) {
    array_t          array;
    Bucket_t**       thread_buckets;

    size_t*          thread_element_counts;
    size_t           buckets_per_thread;
    int              num_threads;

    if(argc != 2) {
        buckets_per_thread = BUCKETS_PER_THREAD;
    } else {
        buckets_per_thread = atoi(argv[1]);
        if(buckets_per_thread < 1) {
            fprintf(stderr, "Nieprawidłowa liczba kubełków na wątek.\n");
            return EXIT_FAILURE;
        }
    }

    array = (array_t)malloc(sizeof(array_element_t) * ARRAY_SIZE);
    if (!array) {
        perror("Alokacja tablicy wejściowej nie powiodła się");
        return EXIT_FAILURE;
    }

    double t_total_s, t_total_e, t_total;
    double t_fill_s, t_fill_e, t_fill;
    double t_distribute_s, t_distribute_e, t_distribute;
    double t_sort_s, t_sort_e, t_sort;
    double t_merge_s, t_merge_e, t_merge;

    MEASURE_TIME(t_total_s);
    #pragma omp parallel
    {
        const int tid = omp_get_thread_num();
        unsigned int seed = tid;

        #pragma omp master
        {
            num_threads = omp_get_num_threads();
            thread_buckets = (Bucket_t**)malloc(sizeof(Bucket_t*) * num_threads);
            thread_element_counts = (size_t*)malloc(sizeof(size_t) * num_threads);
            
            for(int i = 0; i < num_threads; ++i) {
                thread_buckets[i] = (Bucket_t*)malloc(sizeof(Bucket_t) * buckets_per_thread);
            }
        }
        #pragma omp barrier

        // Parallel array filling - each thread fills its own part
        MEASURE_TIME(t_fill_s);
        #pragma omp for
        for(size_t i = 0; i < ARRAY_SIZE; i++) {
            array[i] = rand_r(&seed); // rand_s(&seed); - Windows checking
        }
        MEASURE_TIME(t_fill_e);
        
        #pragma omp barrier // Make sure the array has been filled

        // Each thread initializes its own buckets
        for(size_t i = 0; i < buckets_per_thread; ++i) {
            size_t initial_size = (ARRAY_SIZE / num_threads / buckets_per_thread) * BUCKET_SIZE_OVERHEAD;
            initialize_bucket(&thread_buckets[tid][i], initial_size);
        }

        // The threads' range of values to handle
        const unsigned long long total_range = (unsigned long long)RAND_MAX + 1;
        const unsigned long long range_per_thread = total_range / num_threads;
        const unsigned long long my_min_range = tid * range_per_thread;
        const unsigned long long my_max_range = (tid == num_threads - 1) 
                                                ? total_range 
                                                : (tid + 1) * range_per_thread;


        // Each thread reads the whole array 
        // then distributes the elements to its buckets
        MEASURE_TIME(t_distribute_s);
        for(size_t i = 0; i < ARRAY_SIZE; i++) {
            if((unsigned int)array[i] >= my_min_range && (unsigned int)array[i] < my_max_range) {
                const size_t local_val = (unsigned int)array[i] - my_min_range;
                const size_t bucket_idx = (unsigned long long)local_val * buckets_per_thread / range_per_thread;
                add_element_to_bucket(&thread_buckets[tid][bucket_idx], array[i]);
            }
        }
        MEASURE_TIME(t_distribute_e);

        // Each thread sorts its own buckets
        MEASURE_TIME(t_sort_s);
        size_t my_total_elements = 0;
        for(size_t i = 0; i < buckets_per_thread; i++) {
            qsort(thread_buckets[tid][i].elements, thread_buckets[tid][i].count, sizeof(int), compare_function);
            my_total_elements += thread_buckets[tid][i].count;
        }
        MEASURE_TIME(t_sort_e);
        thread_element_counts[tid] = my_total_elements;

        #pragma omp barrier // Barrier - we wait for threads to calculate their elements

        // Each thread calculates its own starting offset by iterating over the results of previous threads.
        MEASURE_TIME(t_merge_s);
        size_t write_offset = 0;
        for (int i = 0; i < tid; i++) {
            write_offset += thread_element_counts[i];
        }

        // Each thread writes its sorted buckets to the correct position in the output array
        for(size_t i = 0; i < buckets_per_thread; i++) {
            memcpy(array + write_offset, thread_buckets[tid][i].elements, thread_buckets[tid][i].count * sizeof(array_element_t));
            write_offset += thread_buckets[tid][i].count;
        }
        MEASURE_TIME(t_merge_e);

        for(size_t i = 0; i < buckets_per_thread; ++i) {
            free_bucket_elements(&thread_buckets[tid][i]);
        }
    }

    MEASURE_TIME(t_total_e);

    t_total = t_total_e - t_total_s;
    t_fill = t_fill_e - t_fill_s;
    t_distribute = t_distribute_e - t_distribute_s;
    t_sort = t_sort_e - t_sort_s;
    t_merge = t_merge_e - t_merge_s;
    printf("%.15lf;%.15lf;%.15lf;%.15lf;%.15lf\n", t_total, t_fill, t_distribute, t_sort, t_merge);

    if (!array_is_sorted(array)) {
        fprintf(stderr, "Sortowanie nie powiodło się!\n");
    }

    for(int i = 0; i < num_threads; ++i) {
        free(thread_buckets[i]);
    }
    free(thread_buckets);
    free(thread_element_counts);
    free(array);

    return EXIT_SUCCESS;
}