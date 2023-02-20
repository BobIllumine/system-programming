#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libcoro.h"

/**
 * You can compile and run this code using the commands:
 *
 * $> gcc solution.c libcoro.c
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
static void merge_sort(long arr[], int l, int r)
{
	struct coro *this = coro_this();
    if (r > l) {
        int m = l + (r - l) / 2;
        merge_sort(arr, l, m);
        merge_sort(arr, m + 1, r);
        merge(arr, l, m, r);
    }
}

/**
 * Coroutine body. This code is executed by all the coroutines. Here you
 * implement your solution, sort each individual file.
 */
static int coroutine_func_f(void *dummy)
{
	struct coro *this = coro_this();

    char *filename;
    int i;
    // Until all files are sorted
    while(i != FILE_COUNT) {
        if(done[i]) {
            ++i;
            continue;
        }
        else
            filename = strdup(filenames[i]);
        FILE *file = fopen(filename, "r");

        int arr_cnt = 0;
        /* Since we don't know the amount of numbers in the file beforehand, we need to allocate
         * a lot of memory and clean it later
         */
        long *raw_arr = (long *) malloc(1000001 * sizeof(long));
        while (!feof(file))
            fscanf(file, "%ld ", &raw_arr[arr_cnt++]);

        arrays[i] = (long *) malloc(arr_cnt * sizeof(long));
        for (int j = 0; j < arr_cnt; ++j)
            arrays[i][j] = raw_arr[j];

        free(raw_arr);

        merge_sort(arrays[i], 0, arr_cnt - 1);
        done[i] = true;
        lengths[i] = arr_cnt;
        fclose(file);
        ++i;
        coro_yield();
    }
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

    // Allocating memory
    FILE_COUNT = argc - 2;
    filenames = (char **) malloc(FILE_COUNT * sizeof(char *));
    arrays = (long **) malloc(FILE_COUNT * sizeof(long *));
    lengths = (int *) malloc(FILE_COUNT * sizeof(int));
    done = (bool *) malloc(FILE_COUNT * sizeof(bool));

    for(int i = 2; i < argc; ++i) {
        filenames[i - 2] = strdup(argv[i]);
        done[i - 2] = false;
    }
    char *endptr;
	/* Start several coroutines. */
	for (int i = 0; i < strtol(argv[1], &endptr, 10); ++i) {
		coro_new(coroutine_func_f, NULL);
	}
	/* Wait for all the coroutines to end. */
	struct coro *c;
	while ((c = coro_sched_wait()) != NULL) {
		/*
		 * Each 'wait' returns a finished coroutine with which you can
		 * do anything you want. Like check its exit status, for
		 * example. Don't forget to free the coroutine afterwards.
		 */
		printf("Finished %d\n", coro_status(c));
		coro_delete(c);
	}
	/* All coroutines have finished. */

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
    // Output
    for(int i = 0; i < pivot; ++i)
        fprintf(output, "%ld ", sorted_arr[i]);

    // We need to free all dynamically allocated memory
    for(int i = 0; i < FILE_COUNT; ++i) {
        free(arrays[i]);
        free(filenames[i]);
    }
    free(done);
    free(lengths);
    free(sorted_arr);
    free(arrays);
    free(filenames);
    fclose(output);
	return 0;
}
