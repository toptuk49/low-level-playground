#include <stdio.h>
#include <pthread.h>
#include <sys/time.h>

#include "arguments.h"

typedef struct {
    unsigned long long start;
    unsigned long long end;
} Range;

void* threadFunction(void* arg);
unsigned long long factorialRange(Range* range);

unsigned long long result = 1;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
ProgramArguments args = {.k = -1, .pnum = -1, .mod = -1};

int main(int argc, char** argv) {
    if (!parseArguments(&argc, &argv, &args)) return 1;

    pthread_t threads[args.pnum];
    Range ranges[args.pnum];

    unsigned long long chunk = args.k / args.pnum;
    unsigned long long start = 1;

    struct timeval start_time;
    gettimeofday(&start_time, NULL);

    for (int i = 0;  i < args.pnum; i++) {
        ranges[i].start = start;
        ranges[i].end = (i == args.pnum - 1) ? args.k : (start + chunk - 1);
        start = ranges[i].end + 1;

        pthread_create(&threads[i], NULL, threadFunction, &ranges[i]);
    }

    for (int i = 0; i < args.pnum; i++) {
        pthread_join(threads[i], NULL);
    }

    struct timeval finish_time;
    gettimeofday(&finish_time, NULL);

    double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
    elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

    printf("%llu! mod %llu = %llu\n", args.k, args.mod, result);
    printf("Elapsed time: %.2f ms\n", elapsed_time);

    return 0;
}

void* threadFunction(void* arg) {
    Range* range = (Range*)arg;
    unsigned long long partial = factorialRange(range);

    pthread_mutex_lock(&lock);
    result = (result * partial) % args.mod;
    pthread_mutex_unlock(&lock);

    return NULL;
}

unsigned long long factorialRange(Range* range) {
    unsigned long long accumulator = 1;
    for (unsigned long long i = range->start; i <= range->end; i++) {
        accumulator = (accumulator * i) % args.mod;
    }

    return accumulator;
}

