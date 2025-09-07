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
    array_t          array;
    Bucket_t***      thread_buckets;
    
    size_t *thread_element_counts;
    size_t buckets_per_thread;
    int num_threads;

    if (argc != 2) {
        buckets_per_thread = BUCKETS;
    } else {
        buckets_per_thread = atoi(argv[1]);
        if (buckets_per_thread < 1) {
            fprintf(stderr, "Nieprawidłowa liczba kubełków na wątek.\n");
            return EXIT_FAILURE;
        }
    }

    /* -------------------------------------------- */
    /*             STRUCTURES ALLOCATION            */
    /* -------------------------------------------- */
    array = malloc(sizeof(array_element_t) * ARRAY_SIZE);
    if (!array) { perror("malloc"); return EXIT_FAILURE; }

    double t_total_s, t_total_e, t_total;
    double t_fill_s, t_fill_e, t_fill;
    double t_distribute_s, t_distribute_e, t_distribute;
    double t_merge_s, t_merge_e, t_merge;
    double t_sort_s, t_sort_e, t_sort;

    // clear array to assert correct behaviour at the end
    for(size_t i=0; i<ARRAY_SIZE; i++)
    {
        array[i] = 0;
    }

    MEASURE_TIME(t_total_s);

    #pragma omp parallel
    {
        const int tid = omp_get_thread_num();
        unsigned int seed = tid;

        #pragma omp master
        {
            num_threads = omp_get_num_threads();
            thread_buckets = malloc(num_threads * sizeof(Bucket_t **));
            thread_element_counts = calloc(num_threads, sizeof(size_t));
            for (int t = 0; t < num_threads; t++) {
                    thread_buckets[t] = malloc(buckets_per_thread * sizeof(Bucket_t *));
                    for (size_t b = 0; b < buckets_per_thread; b++) {
                        thread_buckets[t][b] = malloc(sizeof(Bucket_t));
                        initialize_bucket(thread_buckets[t][b],
                                        (ARRAY_SIZE / num_threads / buckets_per_thread) * BUCKET_SIZE_OVERHEAD);
                    }
                }
         }
        #pragma omp barrier

        MEASURE_TIME(t_fill_s);
        #pragma omp for
        for (size_t i = 0; i < ARRAY_SIZE; i++) {
            array[i] = rand_r(&seed);
        }
        MEASURE_TIME(t_fill_e);
        
        size_t start = tid * (ARRAY_SIZE / num_threads);
        size_t end = (tid == num_threads - 1) ? ARRAY_SIZE : (tid + 1) * (ARRAY_SIZE / num_threads);

        const unsigned long long total_range = (unsigned long long)RAND_MAX + 1;
        const unsigned long long range_per_bucket = total_range / buckets_per_thread;

        MEASURE_TIME(t_distribute_s);
        for (size_t i = start; i < end; i++) {
            unsigned int val = array[i];
            size_t bucket_idx = val * buckets_per_thread / total_range;
            add_element_to_bucket(thread_buckets[tid][bucket_idx], val);
        }
        MEASURE_TIME(t_distribute_e);

        #pragma omp barrier

        MEASURE_TIME(t_merge_s);
        #pragma omp parallel for
        for (size_t b = 0; b < buckets_per_thread; b++) {
            Bucket_t merged;
            size_t total_count = 0;
            for (int t = 0; t < num_threads; t++)
                total_count += thread_buckets[t][b]->count;

            initialize_bucket(&merged, total_count * BUCKET_SIZE_OVERHEAD);

            for (int t = 0; t < num_threads; t++) {
                memcpy(merged.elements + merged.count, thread_buckets[t][b]->elements,
                       thread_buckets[t][b]->count * sizeof(array_element_t));
                merged.count += thread_buckets[t][b]->count;
            }

            qsort(merged.elements, merged.count, sizeof(array_element_t), compare_function);

            // Zapis do tablicy początkowej
            size_t write_offset = 0;
            for (size_t bb = 0; bb < b; bb++) {
                for (int t = 0; t < num_threads; t++)
                    write_offset += thread_buckets[t][bb]->count;
            }
            memcpy(array + write_offset, merged.elements, merged.count * sizeof(array_element_t));
            free_bucket_elements(&merged);
        }
        MEASURE_TIME(t_merge_e);
    }
    MEASURE_TIME(t_total_e);

    t_total = t_total_e - t_total_s;
    t_fill = t_fill_e - t_fill_s;
    t_distribute = t_distribute_e - t_distribute_s;
    t_merge = t_merge_e - t_merge_s;

    printf("%.15lf;%.15lf;%.15lf;%.15lf\n", t_total, t_fill, t_distribute, t_merge);

    if (!array_is_sorted(array))
        fprintf(stderr, "Sortowanie nie powiodło się!\n");

    // Zwolnienie pamięci
    for (int t = 0; t < num_threads; t++) {
        for (size_t b = 0; b < buckets_per_thread; b++) {
            free_bucket_elements(thread_buckets[t][b]);
            free(thread_buckets[t][b]);
        }
        free(thread_buckets[t]);
    }
    free(thread_buckets);
    free(thread_element_counts);
    free(array);

    return EXIT_SUCCESS;
}
