#include <stdio.h>
#include <stdlib.h>

typedef struct {
    int* elements;
    int count;
    int capacity;
} Bucket;

#define BUCKET_GROWTH_FACTOR 1.2

int resize_bucket(Bucket* bucket) {
    int new_capacity = (int)((double)bucket->capacity * BUCKET_GROWTH_FACTOR);
    int* temp = realloc(bucket->elements, new_capacity * sizeof(int));

    if (temp == NULL) {
        perror("Error: Failed to reallocate memory for bucket");
        return -1;
    }

    bucket->elements = temp;
    bucket->capacity = new_capacity;
    return 0;
}

int add_element_to_bucket(Bucket* bucket, int value) {
   if (bucket->count >= bucket->capacity) {
       if (resize_bucket(bucket) != 0) {
           return -1;
       }
   }

   bucket->elements[bucket->count] = value;
   bucket->count++;
   return 0;
}

void initialize_bucket(Bucket* bucket, long long initial_capacity) {
    bucket->elements = malloc(sizeof(int)*initial_capacity);
    bucket->count = 0;
    bucket->capacity = initial_capacity;
}

void free_bucket(Bucket* bucket) {
    free(bucket->elements);
    free(bucket);
}