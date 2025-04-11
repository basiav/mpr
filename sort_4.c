#include "buckets.h"
#include "omp.h"
#include "time.h"
#include <pthread.h>

#define ARRAY_SIZE 20000000
#define BUCKETS 20000
#define BUCKET_SIZE_OVERHEAD 2 // describes how much more memory should be 
                               // allocated to avoid buckets realocation
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
    unsigned int     seed;
    array_t          array;
    Bucket_t*        buckets;

    /* -------------------------------------------- */
    /*             STRUCTURES ALLOCATION            */
    /* -------------------------------------------- */
    array = (array_t)malloc(sizeof(array_element_t) * ARRAY_SIZE);
    if( !array )
    {
        perror("Array allocation failed");
        return EXIT_FAILURE;
    }

    // clear array to assert correct behaviour at the end
    for(size_t i=0; i<ARRAY_SIZE; i++)
    {
        array[i] = 0;
    }

    buckets = (Bucket_t*)malloc(sizeof(Bucket_t) * BUCKETS);
    if( !buckets )
    {
        free(array);

        perror("Buckets allocation failed");
        return EXIT_FAILURE;
    }

    /* -------------------------------------------- */
    /*           STRUCTURES INITIALIZATION          */
    /* -------------------------------------------- */
    for( size_t i=0; i<BUCKETS; i++ )
    {
        if( initialize_bucket(buckets + i, ARRAY_SIZE * 2 / BUCKETS ) < 0)
        {
            // if bucket allocation failed perform correct teardown and exit
            for( size_t j=0; j<BUCKETS; j++ )
            {
                free_bucket_elements(buckets + j);
            }
            free(array);
            free(buckets);

            perror("Bucket allocation failed");
            return EXIT_FAILURE;
        }
    }

    /* -------------------------------------------- */
    /*                   SORT CODE                  */
    /* -------------------------------------------- */

    // Time measurement variables allocation 
    // txs -> start time for type x
    // txe -> end time for type x
    // tx  -> resulting time for type x
    double ts,  te,  t,
           t1s, t1e, t1,
           t2s, t2e, t2,
           t3s, t3e, t3,
           t4s, t4e, t4;
    ts = omp_get_wtime();

    #pragma omp parallel private(seed)
    {
        seed = omp_get_thread_num();

        // fill the array
        MEASURE_TIME(t1s)
        #pragma omp for
        for(size_t i=0; i<ARRAY_SIZE; i++)
        {
            array[i] = rand_r(&seed);
        }
        MEASURE_TIME(t1e)

        // distribute data to buckets
        MEASURE_TIME(t2s)
        #pragma omp for
        for(size_t i=0; i<ARRAY_SIZE; i++)
        {
            // predefine variables for atomic capture
            const size_t idx = (unsigned long long)array[i] * BUCKETS / ((unsigned long long)RAND_MAX + 1);
            Bucket_t* target_bucket = buckets + idx;
            size_t insertion_index;

            // capture the target value index atomicaly and increase the count
            // because the index is captured atomicaly and count is increased
            // each insertion_index is guaranteed to be unique for different threads
            // meaning no need for mutexes when adding the element.
            #pragma omp atomic capture
            insertion_index = target_bucket->count++;

            target_bucket->elements[insertion_index] = array[i];
        }
        MEASURE_TIME(t2e)

        // sort the buckets
        MEASURE_TIME(t3s)
        #pragma omp for
        for(size_t i=0; i<BUCKETS; i++)
        {
            qsort(buckets[i].elements, buckets[i].count, sizeof(int), compare_function);
        }
        MEASURE_TIME(t3e)

        // fill original array with sorted elements
        MEASURE_TIME(t4s)
        #pragma omp for
        for(size_t i=0; i<BUCKETS; i++)
        {
            // calculate offset for bucket currently being written to array
            const size_t offset = calculate_bucket_final_offset(buckets, i);

            // fill the array
            for(size_t j=0; j<buckets[i].count; j++)
            {
                array[offset + j] = buckets[i].elements[j];
            }
        }
        MEASURE_TIME(t4e)
    }
    te = omp_get_wtime();

    /* -------------------------------------------- */
    /*              ASSERT CORRECTNESS              */
    /* -------------------------------------------- */
    if( !array_is_sorted(array) )
    {
        perror("The resulting array is not sorted");
        return EXIT_FAILURE;
    }

    /* -------------------------------------------- */
    /*              DESTROY STRUCTURES              */
    /* -------------------------------------------- */
    for( size_t i=0; i<BUCKETS; i++ )
    {
        free_bucket_elements(buckets + i);
    }
    free(array);
    free(buckets);

    /* -------------------------------------------- */
    /*            OUTPUT MEASUREMENT DATA           */
    /* -------------------------------------------- */
    t  = te  - ts;
    t1 = t1e - t1s;
    t2 = t2e - t2s;
    t3 = t3e - t3s;
    t4 = t4e - t4s;
    printf("%.15lf;%.15lf;%.15lf;%.15lf;%.15lf\n", t, t1, t2, t3, t4);

    return EXIT_SUCCESS;
}
