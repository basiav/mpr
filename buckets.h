#include <stdio.h>
#include <stdlib.h>

int BUCKET_RESIZING_FACTOR = 2;
int INIT_CAPACITY = 16;

typedef struct {
    int* elements;
    size_t count;
    size_t capacity;
} Bucket_t;

// int add_element_to_bucket(Bucket_t* bucket, int value) {
//     if (bucket->count >= bucket->capacity) {
//         fprintf(stderr, "%ld %ld", bucket->count, bucket->capacity);
//         perror("Error: Algorithm needed to resize the bucket");
//         exit(-1);
//     }

//     bucket->elements[bucket->count] = value;
//     bucket->count++;
//     return 0;
// }
int add_element_to_bucket(Bucket_t* bucket, int value) {
    // Resize the bucket
    if (bucket->count >= bucket->capacity) {
        size_t new_capacity = bucket->capacity * BUCKET_RESIZING_FACTOR;

        int* new_elements = realloc(bucket->elements, sizeof(int) * new_capacity);
        if (!new_elements) {
            perror("Failed to realloc bucket elements");
            exit(EXIT_FAILURE);
        }
        bucket->elements = new_elements;
        bucket->capacity = new_capacity;
    }

    bucket->elements[bucket->count] = value;
    bucket->count++;
    return 0;
}

// int initialize_bucket(Bucket_t* bucket, size_t initial_capacity) {
//     bucket->elements = malloc(sizeof(int)*initial_capacity);
//     if(!bucket->elements) { return -1; }
//     bucket->count = 0;
//     bucket->capacity = initial_capacity;
//     return 0;
// }

int initialize_bucket(Bucket_t* bucket, size_t initial_capacity) {
    // CHANGE: Ensure capacity is never zero.
    size_t real_capacity = initial_capacity > 0 ? initial_capacity : INIT_CAPACITY; // Set a minimum capacity
    bucket->elements = malloc(sizeof(int)*real_capacity);
    
    if(!bucket->elements) { return -1; }
    
    bucket->count = 0;
    bucket->capacity = real_capacity;
    return 0;
}
// int initialize_bucket(Bucket_t* bucket, size_t initial_capacity) {
//     bucket->count = 0;
//     bucket->capacity = initial_capacity > 0 ? initial_capacity : 1; // Unikaj zerowej pojemności
//     bucket->elements = malloc(sizeof(int)*initial_capacity);
//     return bucket->elements != NULL ? 0 : -1;
// }

void free_bucket_elements(Bucket_t* bucket) {
    free(bucket->elements);
}