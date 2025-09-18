#include "omp.h"
#include "stdio.h"
#include "stdlib.h"
#include "time.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE 1000000
#endif

#ifndef SCHEDULER
#define SCHEDULER auto
#endif

#define STR(x) #x
#define TO_STR(x) STR(x)

int main(int argc, char** argv)
{
    int num_threads = omp_get_max_threads();
    unsigned int seeds[num_threads];
    for(int i=0; i<num_threads; i++)
    {
        seeds[i] = time(NULL) ^ (i * 12345);
    }

    int* array = (int*)malloc(sizeof(int) * ARRAY_SIZE);

    // EXPERIMENT
    double start_time, end_time, t;
    start_time = omp_get_wtime();

    #pragma omp parallel private(seeds)
    {
        int thread_id = omp_get_thread_num();

        #pragma omp for schedule(SCHEDULER)
        for(long long i=0; i<ARRAY_SIZE; i++)
        {
            array[i] = rand_r(&seeds[thread_id]); // rand_r / rand_s
        }
    }

    end_time = omp_get_wtime();
    t = ((double) (end_time - start_time));
    // EXPERIMENT END

    printf("%.15lf\n", t);
    free(array);
}
