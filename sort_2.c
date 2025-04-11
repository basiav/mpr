#include "buckets.h"
#include "omp.h"
#include "time.h"
#include <pthread.h>

#define ARRAY_SIZE 20000000
#define BUCKETS 20000
#define BUCKETS_SIZE (ARRAY_SIZE / BUCKETS)

int compare_function(const void *a,const void *b) {
    int *x = (int *) a;
    int *y = (int *) b;
    return *x - *y;
}

int main(int argc, char** argv)
{
    unsigned int seed;
    int* array = (int*)malloc(sizeof(int) * ARRAY_SIZE);
    if( !array )
    {
        return -1;
    }

    // clear array to assert correct behaviour at the end
    for(size_t i=0; i<ARRAY_SIZE; i++)
    {
        array[i] = 0;
    }

    Bucket* buckets = (Bucket*)malloc(sizeof(Bucket) * BUCKETS);
    if( !buckets )
    {
        free(array);
        return -1;
    }

    pthread_mutex_t* buckets_mutexes = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t) * BUCKETS);
    if( !buckets_mutexes )
    {
        free(array);
        free(buckets);
        return -1;
    }

    for( size_t i=0; i<BUCKETS; i++ )
    {
        if( initialize_bucket(buckets + i, BUCKETS_SIZE * 2 ) < 0)
        {
            for( size_t j=0; j<BUCKETS; j++ )
            {
                free_bucket_elements(buckets + j);
                pthread_mutex_destroy(buckets_mutexes + j);
            }
            free(array);
            free(buckets);
            free(buckets_mutexes);
            return -1;
        }

        pthread_mutex_init(buckets_mutexes + i, NULL);
    }

    // EXPERIMENT
    double start_time, end_time, t;
    start_time = omp_get_wtime();

    /* ==================
            SORTING
       ================== */

    #pragma omp parallel private(seed)
    {
        seed = omp_get_thread_num();

        // fill the array
        #pragma omp for schedule(guided, 200)
        for(size_t i=0; i<ARRAY_SIZE; i++)
        {
            array[i] = rand_r(&seed);
        }

        // distribute to buckets
        #pragma omp for schedule(guided, 200)
        for(size_t i=0; i<ARRAY_SIZE; i++)
        {
            const size_t idx = (unsigned long long)array[i] * BUCKETS / ((unsigned long long)RAND_MAX + 1);
            pthread_mutex_lock(buckets_mutexes + idx);
            add_element_to_bucket(buckets + idx, array[i]);
            pthread_mutex_unlock(buckets_mutexes + idx);
        }

        // sort the buckets
        #pragma omp for schedule(auto)
        for(size_t i=0; i<BUCKETS; i++)
        {
            qsort (buckets[i].elements, buckets[i].count, sizeof(int), compare_function);
        }

        // fill original array with sorted elements
        #pragma omp for schedule(auto)
        for(size_t i=0; i<BUCKETS; i++)
        {
            // calculate offset for the bucket
            size_t offset = 0;
            for(size_t j=0; j<i; j++)
            {
                offset += buckets[j].count;
            }

            for(size_t j=0; j<buckets[i].count; j++)
            {
                array[offset + j] = buckets[i].elements[j];
            }
        }

        /* questiones
        a) czy potrzebna jest jakaś ochrona danych wspólnych (tablica początkowa: przy odczycie i przy zapisie; kubełki: przy zapisie, sortowaniu  kubełka, odczycie)?
        b) jaki jest rząd złożoności obliczeniowej algorytmu, a jaka jest praca algorytmu równoległego, czy algorytm jest sekwencyjnie-efektywny?
        */
    }

    end_time = omp_get_wtime();
    t = ((double) (end_time - start_time));
    // EXPERIMENT END

    // assert correctnes
    for( size_t i=0; i<ARRAY_SIZE-1; i++)
    {
        if(array[i] > array[i+1])
        {
            return -1;
        }
    }

    // destroy structures
    for( size_t i=0; i<BUCKETS; i++ )
    {
        free_bucket_elements(buckets + i);
        pthread_mutex_destroy(buckets_mutexes + i);
    }
    free(array);
    free(buckets);
    free(buckets_mutexes);

    printf("%.15lf\n", t);
}
