#include <stdio.h>
#include <stdlib.h>

typedef struct {
    int* elements;
    size_t count;
    size_t capacity;
} Bucket_t;

int add_element_to_bucket(Bucket_t* bucket, int value) {
    if (bucket->count >= bucket->capacity) {
        fprintf(stderr, "%ld %ld", bucket->count, bucket->capacity);
        perror("Error: Algorithm needed to resize the bucket");
        exit(-1);
    }

    bucket->elements[bucket->count] = value;
    bucket->count++;
    return 0;
}

int initialize_bucket(Bucket_t* bucket, size_t initial_capacity) {
    bucket->elements = malloc(sizeof(int)*initial_capacity);
    if(!bucket->elements) { return -1; }
    bucket->count = 0;
    bucket->capacity = initial_capacity;
    return 0;
}

void free_bucket_elements(Bucket_t* bucket) {
    free(bucket->elements);
}