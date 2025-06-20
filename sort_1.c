// #include "buckets.h"
// #include "omp.h"
// #include "time.h"
// #include <pthread.h>
// #include <string.h>

// // #define ARRAY_SIZE 20000000
// #define ARRAY_SIZE 200000
// #define BUCKETS 800 //0
// #define BUCKET_SIZE_OVERHEAD 2 // describes how much more memory should be 
//                                  // allocated to avoid buckets realocation
//                                  // works with values between (1.1 - 1.2)
//                                  // but is unstable
// #define MEASURE_TIME(x) \
// _Pragma("omp master")   \
// x = omp_get_wtime();    \

// typedef int  array_element_t;
// typedef array_element_t* array_t;

// int array_is_sorted(array_t array)
// {
//     /* Checks if the array is sorted
//     Return:
//      - 1 if sorted
//      - 0 if not
//     */

//     for( size_t i=0; i<ARRAY_SIZE-1; i++)
//     {
//         if(array[i] > array[i+1])
//         {
//             return 0;
//         }
//     }
//     return 1;
// }


// int compare_function(const void *a,const void *b)
// {
//     /* Generic comparison function for quicksort used while sorting buckets*/

//     int *x = (int *) a;
//     int *y = (int *) b;
//     return *x - *y;
// }

// size_t calculate_bucket_final_offset(Bucket_t* buckets, size_t bucket_idx)
// {
//     /* calculates the offset of where the bucket should be rewriten at the end */
//     size_t offset = 0;
//     for(size_t j=0; j<bucket_idx; j++)
//     {
//         offset += buckets[j].count;
//     }
//     return offset;
// }

// int main(int argc, char** argv)
// {
//     unsigned int     seed;
//     array_t          array;
//     Bucket_t**       thread_buckets; // Change: 2D array: [thread_id][bucked_id]
//     size_t*          thread_offsets;
//     size_t           buckets_per_thread;
//     size_t*          thread_buckets_counts;
//     int              num_threads;
//     size_t*          thread_element_counts;

//     if( argc != 2 )
//     {
//         buckets_per_thread = BUCKETS;
//     }
//     else
//     {
//         buckets_per_thread = atoi(argv[1]);
//         if( buckets_per_thread < 1 || buckets_per_thread > ARRAY_SIZE )
//         {
//             fprintf(stderr, "Invalid number of buckets\n");
//             return EXIT_FAILURE;
//         }
//     }

//     /* -------------------------------------------- */
//     /*             STRUCTURES ALLOCATION            */
//     /* -------------------------------------------- */
//     array = (array_t)malloc(sizeof(array_element_t) * ARRAY_SIZE);
//     if( !array )
//     {
//         perror("Array allocation failed");
//         return EXIT_FAILURE;
//     }

//     // clear array to assert correct behaviour at the end
//     for(size_t i=0; i<ARRAY_SIZE; i++)
//     {
//         array[i] = 0;
//     }

//     /* -------------------------------------------- */
//     /*                   SORT CODE                  */
//     /* -------------------------------------------- */

//     // Time measurement variables allocation 
//     // txs -> start time for type x
//     // txe -> end time for type x
//     // tx  -> resulting time for type x
//     // double ts,  te,  t,
//     //        t1s, t1e, t1,
//     //        t2s, t2e, t2,
//     //        t3s, t3e, t3,
//     //        t4s, t4e, t4;
//     double ts, t,
//            t1s, t1e, t1,
//            t2s, t2e, t2,
//            t3s, t3e, t3,
//            t4s, t4e, t4;
//     ts = omp_get_wtime();

//     #pragma omp parallel private(seed)
//     {
//         seed = omp_get_thread_num();

//         // Array filling with random numbers
//         MEASURE_TIME(t1s)
//         #pragma omp for
//         for(size_t i=0; i<ARRAY_SIZE; i++)
//         {
//             // array[i] = rand_r(&seed);
//             array[i] = rand();
//         }
//         MEASURE_TIME(t1e)

//         const int tid = omp_get_thread_num();
//         #pragma omp master
//         {
//             num_threads = omp_get_num_threads();
//             thread_buckets = (Bucket_t**)malloc(sizeof(Bucket_t*) * num_threads);
//             thread_offsets = (size_t*)malloc(sizeof(size_t) * num_threads);
//             thread_buckets_counts = (size_t*)malloc(sizeof(size_t) * num_threads);
//             thread_element_counts = (size_t*)malloc(sizeof(size_t) * num_threads);

//             for(int i=0; i<num_threads; ++i) {
//                 thread_buckets[i] = (Bucket_t*)malloc(sizeof(Bucket_t) * buckets_per_thread);
//             }
//         }

//         // Waiting for master to finish allocating thread buckets
//         #pragma omp barrier
        
//         for(size_t i=0; i<buckets_per_thread; ++i) {
//             size_t initial_size = (ARRAY_SIZE / num_threads / buckets_per_thread) * BUCKET_SIZE_OVERHEAD;
//             initialize_bucket(&thread_buckets[tid][i], initial_size);
//         }

//         const unsigned long long range_per_thread = ((unsigned long long)RAND_MAX + 1) / num_threads;
//         const unsigned long long my_min_range = (unsigned long long)tid * range_per_thread;
//         const unsigned long long my_max_range = (tid == num_threads - 1)
//             ? (RAND_MAX + 1) // Ostatni wątek bierze wszystko do końca
//             : (unsigned long long)(tid + 1) * range_per_thread;

//         for(size_t i=0; i<ARRAY_SIZE; i++)
//         {
//             if((unsigned int)array[i] >= my_min_range && (unsigned int)array[i] < my_max_range) {
//                 const size_t local_val = (unsigned int)array[i] - my_min_range;
//                 const size_t bucket_idx = (unsigned long long)local_val * buckets_per_thread / range_per_thread;
//                 add_element_to_bucket(&thread_buckets[tid][bucket_idx], array[i]);
//             }
//         }

//         size_t my_total_elements = 0;
//         for(size_t i=0; i<buckets_per_thread; i++)
//         {
//             qsort(thread_buckets[tid][i].elements, thread_buckets[tid][i].count, sizeof(int), compare_function);
//             my_total_elements += thread_buckets[tid][i].count;
//         }
//         thread_buckets_counts[tid] = my_total_elements;

//         // We have to wait for all thread elements to be counted
//         // before the threads can calculate offsets and write to the input array
//         #pragma omp barrier

//         size_t write_offset = 0;
//         for (int i = 0; i < tid; i++) {
//             write_offset += thread_element_counts[i];
//         }

//         // size_t write_offset = thread_offsets[tid];
//         for(size_t i = 0; i < buckets_per_thread; i++) {
//             memcpy(array + write_offset, thread_buckets[tid][i].elements, thread_buckets[tid][i].count * sizeof(array_element_t));
//             write_offset += thread_buckets[tid][i].count;
//         }

//         // --- Bucket clean-up ---
//         // #pragma omp barrier
//         for(size_t i = 0; i < buckets_per_thread; ++i) {
//             free_bucket_elements(&thread_buckets[tid][i]);
//         }

//     // End of parallel region
//     }

//     double te = omp_get_wtime();

//     // Check correctness and clean-up
//     if (!array_is_sorted(array)) {
//         fprintf(stderr, "Not sorted!\n");
//     } else {
//         printf("Sorting successful.\n");
//     }

//     for(int i = 0; i < num_threads; ++i) {
//         free(thread_buckets[i]);
//     }
//     free(thread_buckets);
//     free(thread_offsets);
//     free(thread_element_counts);
//     free(array);

//     /* -------------------------------------------- */
//     /*            OUTPUT MEASUREMENT DATA           */
//     /* -------------------------------------------- */
//     t  = te  - ts;
//     t1 = t1e - t1s;
//     t2 = t2e - t2s;
//     t3 = t3e - t3s;
//     t4 = t4e - t4s;
//     printf("%.15lf;%.15lf;%.15lf;%.15lf;%.15lf\n", t, t1, t2, t3, t4);

//     return EXIT_SUCCESS;
// }

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "omp.h"
#include "buckets.h"

#define ARRAY_SIZE 20000000
#define BUCKETS 1000           // Domyślna liczba KUBEŁKÓW NA WĄTEK
#define BUCKET_SIZE_OVERHEAD 4 // Mnożnik dla alokacji pamięci w kubełkach

typedef int  array_element_t;
typedef array_element_t* array_t;

// // --- Implementacja struktur i funkcji z pliku buckets.h ---
// typedef struct {
//     int* elements;
//     size_t count;
//     size_t capacity;
// } Bucket_t;

// int initialize_bucket(Bucket_t* bucket, size_t initial_capacity) {
//     bucket->elements = malloc(sizeof(int)*initial_capacity);
//     if(!bucket->elements) { return -1; }
//     bucket->count = 0;
//     bucket->capacity = initial_capacity;
//     return 0;
// }

// int add_element_to_bucket(Bucket_t* bucket, int value) {
//     if (bucket->count >= bucket->capacity) {
//         size_t new_capacity = bucket->capacity * 2;
//         fprintf(stderr, "Zmiana rozmiaru kubełka z %zu na %zu elementów\n", bucket->capacity, new_capacity);
//         int* new_elements = realloc(bucket->elements, sizeof(int) * new_capacity);
//         if (!new_elements) {
//             perror("Błąd reallokacji kubełka");
//             exit(EXIT_FAILURE);
//         }
//         bucket->elements = new_elements;
//         bucket->capacity = new_capacity;
//     }
//     bucket->elements[bucket->count] = value;
//     bucket->count++;
//     return 0;
// }

// void free_bucket_elements(Bucket_t* bucket) {
//     if (bucket->elements) {
//         free(bucket->elements);
//         bucket->elements = NULL;
//     }
// }
// // --- Koniec implementacji z buckets.h ---

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
    // (ZMIANA: Usunięto deklarację `thread_offsets`)
    size_t*          thread_element_counts;
    size_t           buckets_per_thread;
    int              num_threads;

    if(argc != 2) {
        buckets_per_thread = BUCKETS;
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

    unsigned int seed = time(NULL);
    for(size_t i = 0; i < ARRAY_SIZE; i++) {
        // array[i] = rand_r(&seed);
        array[i] = rand();
    }

    double ts = omp_get_wtime();

    #pragma omp parallel
    {
        const int tid = omp_get_thread_num();

        #pragma omp master
        {
            num_threads = omp_get_num_threads();
            thread_buckets = (Bucket_t**)malloc(sizeof(Bucket_t*) * num_threads);
            // (ZMIANA: Usunięto alokację `thread_offsets`)
            thread_element_counts = (size_t*)malloc(sizeof(size_t) * num_threads);

            for(int i = 0; i < num_threads; ++i) {
                thread_buckets[i] = (Bucket_t*)malloc(sizeof(Bucket_t) * buckets_per_thread);
            }
        }
        #pragma omp barrier

        for(size_t i = 0; i < buckets_per_thread; ++i) {
            size_t initial_size = (ARRAY_SIZE / num_threads / buckets_per_thread) * BUCKET_SIZE_OVERHEAD;
            initialize_bucket(&thread_buckets[tid][i], initial_size);
        }

        const unsigned long long range_per_thread = ((unsigned long long)RAND_MAX + 1) / num_threads;
        const unsigned long long my_min_range = (unsigned long long)tid * range_per_thread;
        const unsigned long long my_max_range = (tid == num_threads - 1)
            ? ((unsigned long long)RAND_MAX + 1)
            : (unsigned long long)(tid + 1) * range_per_thread;

        for(size_t i = 0; i < ARRAY_SIZE; i++) {
            if((unsigned int)array[i] >= my_min_range && (unsigned int)array[i] < my_max_range) {
                const size_t local_val = (unsigned int)array[i] - my_min_range;
                const size_t bucket_idx = (unsigned long long)local_val * buckets_per_thread / range_per_thread;
                add_element_to_bucket(&thread_buckets[tid][bucket_idx], array[i]);
            }
        }

        size_t my_total_elements = 0;
        for(size_t i = 0; i < buckets_per_thread; i++) {
            qsort(thread_buckets[tid][i].elements, thread_buckets[tid][i].count, sizeof(int), compare_function);
            my_total_elements += thread_buckets[tid][i].count;
        }
        thread_element_counts[tid] = my_total_elements;

        #pragma omp barrier // KRYTYCZNA BARIERA: Czekamy, aż wszystkie wątki policzą swoje elementy

        // (ZMIANA: Usunięto blok #pragma omp master i następującą po nim barierę)
        // --- Obliczanie offsetów (wersja "każdy dla siebie") ---
        // Każdy wątek sam oblicza swój offset startowy, iterując po wynikach poprzednich wątków.
        size_t write_offset = 0;
        for (int i = 0; i < tid; i++) {
            write_offset += thread_element_counts[i];
        }

        // --- Zapis do tablicy wynikowej ---
        // Każdy wątek przepisuje swoje posortowane kubełki do właściwego, samodzielnie obliczonego miejsca
        for(size_t i = 0; i < buckets_per_thread; i++) {
            memcpy(array + write_offset, thread_buckets[tid][i].elements, thread_buckets[tid][i].count * sizeof(array_element_t));
            write_offset += thread_buckets[tid][i].count;
        }

        for(size_t i = 0; i < buckets_per_thread; ++i) {
            free_bucket_elements(&thread_buckets[tid][i]);
        }
    }

    double te = omp_get_wtime();

    if (!array_is_sorted(array)) {
        fprintf(stderr, "Sortowanie nie powiodło się!\n");
    } else {
        printf("Sortowanie zakończone sukcesem.\n");
    }

    for(int i = 0; i < num_threads; ++i) {
        free(thread_buckets[i]);
    }
    free(thread_buckets);
    // (ZMIANA: Usunięto zwalnianie `thread_offsets`)
    free(thread_element_counts);
    free(array);

    printf("Całkowity czas sortowania: %.15lf s\n", te - ts);

    return EXIT_SUCCESS;
}