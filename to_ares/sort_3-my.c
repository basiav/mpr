#include "buckets.h"
#include "omp.h"
#include "time.h"
#include <pthread.h>

#define ARRAY_SIZE 20000000
#define BUCKETS 8000
#define BUCKET_SIZE_OVERHEAD 2 // describes how much more memory should be 
                                 // allocated to avoid buckets realocation
                                 // works with values between (1.1 - 1.2)
                                 // but is unstable
#define MEASURE_TIME(x) \
_Pragma("omp master")   \
x = omp_get_wtime();    \

typedef int  array_element_t;
typedef array_element_t* array_t;

int array_is_sorted(array_t array)
{
    /* Checks if the array is sorted
    Return:
     - 1 if sorted
     - 0 if not
    */

    for( size_t i=0; i<ARRAY_SIZE-1; i++)
    {
        if(array[i] > array[i+1])
        {
            return 0;
        }
    }
    return 1;
}

int compare_function(const void *a,const void *b)
{
    /* Generic comparison function for quicksort used while sorting buckets*/

    int *x = (int *) a;
    int *y = (int *) b;
    return *x - *y;
}

size_t calculate_bucket_final_offset(Bucket_t* buckets, size_t bucket_idx)
{
    /* calculates the offset of where the bucket should be rewriten at the end */
    size_t offset = 0;
    for(size_t j=0; j<bucket_idx; j++)
    {
        offset += buckets[j].count;
    }
    return offset;
}



int main(int argc, char** argv)
{
    array_t array;
    Bucket_t*** thread_buckets; // Buckets to which the threads fill the array values
    Bucket_t* buckets; // Merged buckets to be sorted
    size_t buckets_per_thread; // How many buckets each thread has

    unsigned int seed;
    int num_threads;

    if( argc != 2 )
    {
        buckets_per_thread = BUCKETS;
    }
    else
    {
        buckets_per_thread = atoi(argv[1]);
        if( buckets_per_thread < 1 || buckets_per_thread > ARRAY_SIZE )
        {
            fprintf(stderr, "Invalid number of buckets\n");
            return EXIT_FAILURE;
        }
    }

    // ARRAY
    // Array allocation
    array = (array_t)malloc(sizeof(array_element_t) * ARRAY_SIZE);
    if( !array )
    {
        perror("Array allocation failed");
        return EXIT_FAILURE;
    }

    // Clear array to assert correct behaviour at the end
    for(size_t i=0; i<ARRAY_SIZE; i++)
    {
        array[i] = 0;
    }

    // Master thread allocates the buckets
    num_threads = omp_get_num_threads();
    thread_buckets = (Bucket_t***)malloc(sizeof(Bucket_t**) * num_threads);
    if( !thread_buckets )
    {
        free(array);

        perror("Buckets allocation failed");
        return EXIT_FAILURE;
    }
    for (int t = 0; t < num_threads; t++) {
        thread_buckets[t] = (Bucket_t**)malloc(buckets_per_thread * sizeof(Bucket_t *));
    }

    // Allocation of the buckets with merged values from thread_buckets
    buckets = malloc(buckets_per_thread * sizeof(Bucket_t));
    if (!buckets) {
        perror("Buckets allocation failed");
        free(array);

        return EXIT_FAILURE;
    }

    // Time measurements
    double t_total_s, t_total_e, t_total;
    double t_fill_s, t_fill_e, t_fill;
    double t_distribute_s, t_distribute_e, t_distribute;
    double t_merge_buckets_s, t_merge_buckets_e, t_merge_buckets;
    double t_sort_s, t_sort_e, t_sort;
    double t_merge_arr_s, t_merge_arr_e, t_merge_arr;

    MEASURE_TIME(t_total_s);

    #pragma omp parallel private(seed)
    {
        seed = omp_get_thread_num();
        int tid = seed;

        // Fill the array - each thread its part
        MEASURE_TIME(t_fill_s)
        #pragma omp for
        for(size_t i=0; i<ARRAY_SIZE; i++)
        {
            array[i] = rand_s(&seed); //rand_r(&seed);
        }
        MEASURE_TIME(t_fill_e)

        // Each thread allocates its buckets
        size_t statistic_init_bucket_size = (ARRAY_SIZE / num_threads / buckets_per_thread) * BUCKET_SIZE_OVERHEAD;
        for (size_t b = 0; b < buckets_per_thread; b++) {
            thread_buckets[tid][b] = malloc(sizeof(Bucket_t));
            initialize_bucket(thread_buckets[tid][b], statistic_init_bucket_size);
        }
        #pragma omp barrier // ??

        // Each thread reads the array 
        unsigned long long normalizing_values_factor = (unsigned long long)RAND_MAX + 1;
        MEASURE_TIME(t_distribute_s)
        #pragma omp for
        for (size_t i = 0; i < ARRAY_SIZE; i++) {
            unsigned int val = array[i];
            size_t bucket_idx = val * buckets_per_thread / normalizing_values_factor;
            add_element_to_bucket(thread_buckets[tid][bucket_idx], val);
        }
        MEASURE_TIME(t_distribute_e)

        // Merge the buckets from all threads
        MEASURE_TIME(t_merge_buckets_s)
        #pragma omp for
        for (size_t b = 0; b < buckets_per_thread; b++) {
            // Calculate the size of a bucket in merged buckets
            size_t total_count = 0;
            for (int t = 0; t < num_threads; t++) {
                total_count += thread_buckets[t][b]->count;
            }
            initialize_bucket(&buckets[b], total_count);

            size_t offset = 0;
            for (int t = 0; t < num_threads; t++) {
                Bucket_t* src_bucket = thread_buckets[t][b];
                for (size_t j = 0; j < src_bucket->count; j++) {
                    buckets[b].elements[offset++] = thread_buckets[t][b]->elements[j];
                }
            }
            buckets[b].count = total_count;
        }
        MEASURE_TIME(t_merge_buckets_e)

        // Sort the buckets
        MEASURE_TIME(t_sort_s)
        #pragma omp for
        for (size_t i = 0; i < buckets_per_thread; i++) {
            qsort(buckets[i].elements, buckets[i].count, sizeof(int), compare_function);
        }
        MEASURE_TIME(t_sort_e);

        // Fill the original array
        MEASURE_TIME(t_fill_s)
        #pragma omp for
        for (size_t i = 0; i < buckets_per_thread; i++) {
            const size_t offset = calculate_bucket_final_offset(buckets, i);

            for (size_t j = 0; j < buckets[i].count; j++) {
                array[offset + j] = buckets[i].elements[i];
            }
        }
        MEASURE_TIME(t_fill_e);
    }
    MEASURE_TIME(t_total_e);

    // Check if array sorted
    if( !array_is_sorted(array) )
    {
        perror("The resulting array is not sorted");
        return EXIT_FAILURE;
    }

    // Free memory
    for( size_t i=0; i<buckets_per_thread; i++ )
    {
        free_bucket_elements(buckets + i);
    }
    for (size_t t = 0; t < num_threads; t++) {
        for (size_t j = 0; j < buckets_per_thread; j++) {
            free_bucket_elements(thread_buckets[t][j]);
            free(thread_buckets[t][j]);
        }
        free(thread_buckets[t]);
    }
    free(array);
    free(thread_buckets);
    free(buckets);

    t_total = t_total_e - t_total_s;
    t_fill = t_fill_e - t_fill_s;
    t_distribute = t_distribute_e - t_distribute_s;
    t_merge_buckets = t_merge_buckets_e - t_merge_buckets_s;
    t_sort = t_sort_e - t_sort_s;
    t_merge_arr = t_merge_arr_e - t_merge_arr_s;

    // Standarized output in our code - t_merge
    // Additional output here - t_merge_buckets - at the end
    printf("%.15lf;%.15lf;%.15lf;%.15lf;%.15lf;%.15lf\n", t_total, t_fill, t_distribute, t_sort, t_merge_arr, t_merge_buckets);

    return EXIT_SUCCESS;
}
