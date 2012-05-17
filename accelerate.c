// clang accelerate.c -framework Accelerate -o accelerate && ./accelerate

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <Accelerate/Accelerate.h>

typedef void (^Block)();

#ifdef __arm__
  #define BENCHMARK_COUNT 5
#else
  #define BENCHMARK_COUNT 50
#endif

#define RUN_POPULATION_BENCHMARKS
#define RUN_SCALING_BENCHMARKS
#define RUN_SUMMING_BENCHMARKS
#define RUN_SEARCH_BENCHMARKS

float benchmark(char *caption, Block block, int times) {
    struct timeval start, end;
    printf("%s: ", caption);
    
    gettimeofday(&start, NULL);
    for (int i = 0; i < times; i++)
        block();
    gettimeofday(&end, NULL);
    
    float msec = ((end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec)) / 1000.0;
    msec /= times;
    printf("%f msec\n", msec);
    return msec;
}

float *makeArray(int length) {
    float *array = malloc(sizeof(float) * length);
    catlas_sset(length, 10, array, 1);
    return array;
}

int main(int argc, char **argv)
{
    struct timeval start, end;
    gettimeofday(&start, NULL);
    
    int sample1, sample2;

    #ifdef RUN_POPULATION_BENCHMARKS
    {
        printf("\n\narray population, increasing value\n\n");
        
        float *myArray = malloc(10000000 * sizeof(float));
        sample1 = benchmark("populating a 10000000-element float array (C)", ^{
            for (int i = 0; i < 10000000; i++) {
                myArray[i] = i;
            }
        }, BENCHMARK_COUNT);

        sample2 = benchmark("populating a 10000000-element float array (catlas)", ^{
            for (int i = 0; i < 10000000; i++) {
                float f = i;
                catlas_sset(1, f, &(myArray[i]), 1);
            }
        }, BENCHMARK_COUNT);
        printf("CATLAS took %f%% execution time.\n", ((float)sample2 / (float)sample1) * 100);

        sample2 = benchmark("populating a 10000000-element float array (vDSP)", ^{
          float initial = 0;
          float increment = 1;
          vDSP_vramp(&initial, &increment, myArray, 1, 10000000);
        }, BENCHMARK_COUNT);
        printf("vDSP took %f%% execution time.\n", ((float)sample2 / (float)sample1) * 100);

    
        printf("\n\narray population, constant value\n\n");

        sample1 = benchmark("populating a 10000000-element float array", ^{
            for (int i = 0; i < 10000000; i++) {
                myArray[i] = 10;
            }
        }, BENCHMARK_COUNT);

        sample2 = benchmark("populating a 10000000-element float array (catlas)", ^{
            catlas_sset(10000000, 10, myArray, 1);
        }, BENCHMARK_COUNT);
        printf("CATLAS took %f%% execution time.\n", ((float)sample2 / (float)sample1) * 100);

        sample2 = benchmark("populating a 10000000-element float array (vDSP)", ^{
            float initial = 10;
            float increment = 0;
            vDSP_vramp(&initial, &increment, myArray, 1, 10000000);
        }, BENCHMARK_COUNT);
        printf("vDSP took %f%% execution time.\n", ((float)sample2 / (float)sample1) * 100);
        
        free(myArray);
    }
    #endif
    
    #ifdef RUN_SCALING_BENCHMARKS
    {
        printf("\n\nscaling benchmarks\n\n");
        
        float *myArray = makeArray(10000000);
        
        sample1 = benchmark("doubling a 10000000-element float array", ^{
            for (int i = 0; i < 10000000; i++) { myArray[i] *= 2; }
        }, BENCHMARK_COUNT);

        sample2 = benchmark("doubling a 10000000-element float array (blas)", ^{
            cblas_sscal(10000000, 2, myArray, 1);
        }, BENCHMARK_COUNT);
        printf("BLAS took %f%% execution time.\n", ((float)sample2 / (float)sample1) * 100);

        sample1 = benchmark("doubling a 10000000-element float array (20x)", ^{
            for (int h = 0; h < 20; h++)
                for (int i = 0; i < 10000000; i++) { myArray[i] *= 2; }
        }, BENCHMARK_COUNT);

        sample2 = benchmark("doubling a 10000000-element float array (blas, 20x)", ^{
            for (int i = 0; i < 20; i++)
                cblas_sscal(10000000, 2, myArray, 1);
        }, BENCHMARK_COUNT);
        printf("BLAS took %f%% execution time.\n", ((float)sample2 / (float)sample1) * 100);
        
        free(myArray);
    }
    #endif

    #ifdef RUN_SUMMING_BENCHMARKS
    {
        printf("\n\nsumming benchmarks (the right answer is 49999995000000)\n\n");
        // (0..10000000-1).to_a.reduce {|a,b| a+b} => 49999995000000
        
        float *floatArray = malloc(10000000 * sizeof(float));
        for (int i = 0; i < 10000000; i++) { floatArray[i] = i; }
        double *doubleArray = malloc(10000000 * sizeof(double));
        for (int i = 0; i < 10000000; i++) { doubleArray[i] = i; }

        #ifdef RUN_OBJC
        {
            NSMutableArray *objcArray = [[NSMutableArray alloc] initWithCapacity:10000000];
            for (int i = 0; i < 10000000; i++) {
                NSNumber *number = [[NSNumber alloc] initWithFloat:floatArray[i]];
                [objcArray addObject:number];
                [number release];
            }
            benchmark("calculate sum of 10000000-element float array (Objective-C)", ^{
                float sum = 0;
                for (int i = 0; i < 10000000; i++)
                    sum += fabs([[objcArray objectAtIndex:i] floatValue]);
                printf("\n ..sum is %f.. ", sum);
            }, 1);
            [objcArray release];
        }
        #endif
        
        sample1 = benchmark("calculate sum of 10000000-element float array", ^{
            float sum = 0;
            for (int i = 0; i < 10000000; i++) { sum += fabs(floatArray[i]); }
            printf("\n ..sum is %f.. ", sum);
        }, BENCHMARK_COUNT);

        sample2 = benchmark("calculate sum of 10000000-element float array (blas)", ^{
            float sum = cblas_sasum(10000000, floatArray, 1);
            printf("\n ..sum is %f.. ", sum);
        }, BENCHMARK_COUNT);
        printf("BLAS took %f%% execution time.\n", ((float)sample2 / (float)sample1) * 100);
        
        sample1 = benchmark("calculate sum of 10000000-element double array", ^{
            double sum = 0;
            for (int i = 0; i < 10000000; i++) { sum += abs(doubleArray[i]); }
            printf("\n ..sum is %f.. ", sum);
        }, BENCHMARK_COUNT);

        sample2 = benchmark("calculate sum of 10000000-element double array (blas)", ^{
            double sum = cblas_dasum(10000000, doubleArray, 1);
            printf("\n ..sum is %f.. ", sum);
        }, BENCHMARK_COUNT);
        printf("BLAS took %f%% execution time.\n", ((float)sample2 / (float)sample1) * 100);
        
        free(floatArray);
        free(doubleArray);
    }
    #endif

    #ifdef RUN_SEARCH_BENCHMARKS
    {
        printf("\n\nsearch benchmarks\n\n");
        
        float *myArray = malloc(10000000 * sizeof(float));
        for (int i = 0; i < 10000000; i++) { myArray[i] = (float)arc4random(); }

        sample1 = benchmark("search 10000000-element float array", ^{
            int maxposition = 0;
            for (int i = 0; i < 10000000; i++)
                if (fabs(myArray[i]) > fabs(myArray[maxposition])) maxposition = i;
            printf("\n ..max position is %d.. ", maxposition);
        }, BENCHMARK_COUNT);

        sample2 = benchmark("search 10000000-element float array (blas)", ^{
            int maxposition = cblas_isamax(10000000, myArray, 1);
            printf("\n ..max position is %d.. ", maxposition);
        }, BENCHMARK_COUNT);
        printf("BLAS took %f%% execution time.\n", ((float)sample2 / (float)sample1) * 100);
        
        free(myArray);
    }
    #endif
    
    gettimeofday(&end, NULL);
    float msec = ((end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec)) / 1000.0;
    printf("\n\nall benchmarks took %f msec\n", msec);
    
    return 0;
}


