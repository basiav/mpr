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
    Bucket_t*** thread_buckets;
    size_t buckets_per_thread;

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

    thread_buckets = (Bucket_t***)malloc(sizeof(Bucket_t**) * num_threads);
    if( !thread_buckets )
    {
        free(array);

        perror("Buckets allocation failed");
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
            array[i] = rand_r(&seed);
        }
        MEASURE_TIME(t_fill_e)

        

    }

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
