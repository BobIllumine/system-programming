#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "libcoro.h"

/**
 * You can compile and run this code using the commands:
 *
 * $> gcc solution.c libcoro.c -lm
 * $> ./a.out
 */

int FILE_COUNT;
int *lengths;
long **arrays;
char **filenames;
bool *done;
/**
 * Merge part for merge sort function. Reference: https://www.geeksforgeeks.org/merge-sort/
 * @param arr array to merge
 * @param l left index
 * @param m mid index
 * @param r right index
 */
static void merge(long arr[], int l, int m, int r)
{
    int size_l = m - l + 1,
        size_r = r - m;

    // Allocating memory dynamically because stack size is too small
    long *arr_l = (long *)malloc(sizeof(long) * size_l),
        *arr_r = (long *)malloc(sizeof(long) * size_r);

    for(int i = 0; i < size_l; ++i)
        arr_l[i] = arr[l + i];
    for(int j = 0; j < size_r; ++j)
        arr_r[j] = arr[m + j + 1];

    int i = 0, j = 0, k = l;
    while(i < size_l && j < size_r) {
        if(arr_l[i] <= arr_r[j]) {
            arr[k] = arr_l[i];
            ++i;
        }
        else {
            arr[k] = arr_r[j];
            ++j;
        }
        ++k;
    }
    while(i < size_l) {
        arr[k] = arr_l[i];
        ++i, ++k;
    }

    while(j < size_r) {
        arr[k] = arr_r[j];
        ++j, ++k;
    }
    free(arr_l);
    free(arr_r);
}

/**
 * Merge sort function. Reference: https://www.geeksforgeeks.org/merge-sort/
 * @param arr initial array
 * @param l start index
 * @param r end index
 */
static void merge_sort(double *timer, long arr[], int l, int r)
{
    clock_t start = clock();
    if (r > l) {
        int m = l + (r - l) / 2;
        *timer += (double)(clock() - start);
        merge_sort(timer, arr, l, m);
        coro_yield();
        start = clock();
        *timer += (double)(clock() - start);
        merge_sort(timer, arr, m + 1, r);
        coro_yield();
        start = clock();
        merge(arr, l, m, r);
        *timer += (double)(clock() - start);
        coro_yield();
    }
}

/**
 * Coroutine body. This code is executed by all the coroutines. Here you
 * implement your solution, sort each individual file.
 */
static int coroutine_func_f(void *context)
{
    double total_time = 0;
    struct coro *this = coro_this();
    char *name = context;
    printf("Started %s\n", name);
    clock_t coro_start = clock();
    int i = 0;
    // Until all files are sorted
    while(i != FILE_COUNT) {
        coro_start = clock();
        FILE *file;
        if(done[i]) {
            ++i;
            continue;
        }
        else
            file = fopen(filenames[i], "r");

        int arr_cnt = 0;
        /* Since we don't know the amount of numbers in the file beforehand, we need to allocate
         * a lot of memory and clean it later
         */
        long *raw_arr = (long *) malloc(10 * sizeof(long));
        int cur_size = 10;
        while(!feof(file)) {
            if(arr_cnt == cur_size - 1) {
                raw_arr = realloc(raw_arr, cur_size * 2 * sizeof(long));
                cur_size *= 2;
            }
            fscanf(file, "%ld ", &raw_arr[arr_cnt++]);
        }

        arrays[i] = (long *) malloc(arr_cnt * sizeof(long));
        for (int j = 0; j < arr_cnt; ++j)
            arrays[i][j] = raw_arr[j];

        free(raw_arr);

        done[i] = true;
        lengths[i] = arr_cnt;
        total_time += (double)(clock() - coro_start);
        merge_sort(&total_time, arrays[i], 0, arr_cnt - 1);
        coro_start = clock();
        fclose(file);
        ++i;
        total_time += (double)(clock() - coro_start);
        coro_yield();
    }
    printf("%s: execution time -- %.1fus\n", name, total_time * 1e6 / CLOCKS_PER_SEC);
    free(name);
	return 0;
}

/**
 * Entrypoint function. Pass input file names and output file name as a command line argument
 */
int main(int argc, char **argv)
{
    if(argc < 3) {
        printf("Use: $ gcc solution.c -o main <COROUTINES_NUM> <INPUT_FILE1> <INPUT_FILE2> ... <INPUT_FILEN>");
        return 0;
    }
    /* Initialize our coroutine global cooperative scheduler. */
    coro_sched_init();
    clock_t start = clock();
    char *endptr;
    // Allocating memory
    FILE_COUNT = argc - 2;
    filenames = (char **) malloc(FILE_COUNT * sizeof(char *));
    arrays = (long **) malloc(FILE_COUNT * sizeof(long *));
    lengths = (int *) malloc(FILE_COUNT * sizeof(int));
    done = (bool *) malloc(FILE_COUNT * sizeof(bool));

    for(int i = 2; i < argc; ++i) {
        filenames[i - 2] = argv[i];
        done[i - 2] = false;
    }

    long coroutines = strtol(argv[1], &endptr, 10);

	/* Start several coroutines. */
	for (long i = 0; i < coroutines; ++i) {
        // Calculating the length of the coroutine name
        long size = (i ? (long)ceil(log10(i)) + 7 : 8);
        char *name = (char*)malloc(size * sizeof(char));
        sprintf(name, "coro_%ld", i);
		coro_new(coroutine_func_f, strdup(name));
        free(name);
	}
	/* Wait for all the coroutines to end. */
	struct coro *c;
	while ((c = coro_sched_wait()) != NULL) {
		/*
		 * Each 'wait' returns a finished coroutine with which you can
		 * do anything you want. Like check its exit status, for
		 * example. Don't forget to free the coroutine afterwards.
		 */
        printf("Switch count -- %lld\n", coro_switch_count(c));
		printf("Finished %d\n", coro_status(c));
		coro_delete(c);
	}
	/* All coroutines have finished. */

    clock_t merge_start = clock();
    // Output file
    FILE* output = fopen("output.txt", "w");

    /*
     * Since all arrays are sorted at this point, we can use `merge()` function in order to merge them quickly (O(N) time complexity)
     * The only alteration is that mid index now should contain the first index of second array.
     * Each iteration we will merge previously merged array with the next one
     */
    int pivot = lengths[0];
    long *sorted_arr = (long *) malloc(sizeof(long) * pivot);
    memcpy(sorted_arr, arrays[0], pivot * sizeof(long));
    for(int i = 1; i < FILE_COUNT; ++i) {
        // Reallocate some more memory
        long *tmp = realloc(sorted_arr, (lengths[i] + pivot) * sizeof(long));
        // If reallocation fails
        if(tmp == NULL) {
            printf("oops... something went wrong te-he ;P");
            return -1;
        }
        else {
            sorted_arr = tmp;
            // Concatenate arrays
            for(int j = 0; j < lengths[i]; ++j)
                sorted_arr[pivot + j] = arrays[i][j];
            // Merge them
            merge(sorted_arr, 0, pivot - 1, pivot + lengths[i] - 1);
            // Update total length
            pivot += lengths[i];
        }
    }
    clock_t finish = clock();
    // Output
    for(int i = 0; i < pivot; ++i)
        fprintf(output, "%ld ", sorted_arr[i]);
    // We need to free all dynamically allocated memory
    for(int i = 0; i < FILE_COUNT; ++i)
        free(arrays[i]);
    free(arrays);
    free(done);
    free(lengths);
    free(sorted_arr);
    free(filenames);
    fclose(output);
    printf("Final merge: %.1fus\n", (double)(finish - merge_start) * 1e6 / CLOCKS_PER_SEC);
    printf("Execution time: %.1fus", (double)(finish - start) * 1e6 / CLOCKS_PER_SEC);
	return 0;
}
