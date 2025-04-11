#include "buckets.h"
#include "omp.h"
#include "time.h"
#include <pthread.h>

#define ARRAY_SIZE 20000000
#define BUCKETS 20000
#define BUCKET_SIZE_OVERHEAD 2 // describes how much more memory should be 
                               // allocated to avoid buckets realocation

typedef int  array_element_t;
typedef array_element_t* array_t;

int array_is_sorted(array_t array)
{
    /* Checks if the array is sorted
    Return:
     - 0 if sorted
     - -1 if not
    */

    for( size_t i=0; i<ARRAY_SIZE-1; i++)
    {
        if(array[i] > array[i+1])
        {
            return -1;
        }
    }
    return 0;
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
    pthread_mutex_t* buckets_mutexes;

    /* -------------------------------------------- */
    /*             STRUCTURES ALLOCATION            */
    /* -------------------------------------------- */
    array = (array_t)malloc(sizeof(array_element_t) * ARRAY_SIZE);
    if( !array )
    {
        perror("Array allocation failed");
        return -1;
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
        return -1;
    }

    buckets_mutexes = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t) * BUCKETS);
    if( !buckets_mutexes )
    {
        free(array);
        free(buckets);

        perror("Mutex list allocation failed");
        return -1;
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
                pthread_mutex_destroy(buckets_mutexes + j);
            }
            free(array);
            free(buckets);
            free(buckets_mutexes);

            perror("Bucket allocation failed");
            return -1;
        }

        pthread_mutex_init(buckets_mutexes + i, NULL);
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
        t1s = omp_get_wtime();
        #pragma omp for
        for(size_t i=0; i<ARRAY_SIZE; i++)
        {
            array[i] = rand_r(&seed);
        }
        t1e = omp_get_wtime();

        // distribute data to buckets
        t2s = omp_get_wtime();
        #pragma omp for
        for(size_t i=0; i<ARRAY_SIZE; i++)
        {
            // calculate desired bucket index
            const size_t idx = (unsigned long long)array[i] * BUCKETS / ((unsigned long long)RAND_MAX + 1);
            
            // lock desired bucket mutex and than add value to the bucket
            pthread_mutex_lock(buckets_mutexes + idx);
            add_element_to_bucket(buckets + idx, array[i]);
            pthread_mutex_unlock(buckets_mutexes + idx);
        }
        t2e = omp_get_wtime();

        // sort the buckets
        t3s = omp_get_wtime();
        #pragma omp for
        for(size_t i=0; i<BUCKETS; i++)
        {
            qsort(buckets[i].elements, buckets[i].count, sizeof(int), compare_function);
        }
        t3e = omp_get_wtime();

        // fill original array with sorted elements
        t4s = omp_get_wtime();
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
        t4e = omp_get_wtime();
    }
    te = omp_get_wtime();

    /* -------------------------------------------- */
    /*              ASSERT CORRECTNESS              */
    /* -------------------------------------------- */
    for( size_t i=0; i<ARRAY_SIZE-1; i++)
    {
        if(array[i] > array[i+1])
        {
            return -1;
        }
    }

    /* -------------------------------------------- */
    /*              DESTROY STRUCTURES              */
    /* -------------------------------------------- */
    for( size_t i=0; i<BUCKETS; i++ )
    {
        free_bucket_elements(buckets + i);
        pthread_mutex_destroy(buckets_mutexes + i);
    }
    free(array);
    free(buckets);
    free(buckets_mutexes);

    /* -------------------------------------------- */
    /*            OUTPUT MEASUREMENT DATA           */
    /* -------------------------------------------- */
    t  = te  - ts;
    t1 = t1e - t1s;
    t2 = t2e - t2s;
    t3 = t3e - t3s;
    t4 = t4e - t4s;
    printf("%.15lf;%.15lf;%.15lf;%.15lf;%.15lf\n", t, t1, t2, t3, t4);

    return 0;
}
