#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#define MAX_ENTRIES 100000
#define MAX_NAME 5
#define MAX_ID 40000000
#define THRESHOLD 42
#define MAX_THREADS 16

typedef struct
{
    char name[MAX_NAME];
    int id;
    char timestamp[20];
} Entry;

Entry entries[MAX_ENTRIES];
Entry sorted_entries[MAX_ENTRIES];
int counting_array[MAX_ID + 1] = {0};
int prefix_sum[MAX_ID + 1] = {0};
int entry_count;
int num_threads;
pthread_mutex_t lock;

typedef struct
{
    Entry *files;
    int *count;
    int start_idx;
    int end_idx;
    const char *column;
} ThreadArgs;

int final_count[MAX_ID];
pthread_mutex_t count_mutex;

// Function prototypes
int compare_entries(const Entry *a, const Entry *b);
void *count_elements(void *arg);
void calculate_prefix_sum();
void *place_elements(void *arg);
void parallel_count_sort();
void merge_sort_recursive(Entry arr[], int l, int r);
void merge(Entry arr[], int l, int m, int r);
void *merge_sort_thread(void *arg);
void parallel_merge_sort();
double get_time_in_seconds(clock_t start, clock_t end);

// Thread data structure
typedef struct
{
    int start;
    int end;
} ThreadData;

int sort_criteria = 2;

int compare_entries(const Entry *a, const Entry *b)
{
    if (sort_criteria == 0)
    {
        // Sort by Name (alphabetically)
        return strcmp(a->name, b->name);
    }
    else if (sort_criteria == 1)
    {
        // Sort by ID (numerically)
        return a->id - b->id;
    }
    else if (sort_criteria == 2)
    {
        // Sort by Timestamp (ISO 8601 format)
        return strcmp(a->timestamp, b->timestamp);
    }
    return 0;
}

int BASE = 26;

unsigned long long hash_string(const char *str)
{
    unsigned long long hash = 0;
    int len = strlen(str);

    for (int i = 0; i < MAX_NAME; i++)
    {
        int value = 0; // Default value for padding
        if (i < len)
        {
            value = str[i] - 'a' + 1; // 'a' -> 1, ..., 'z' -> 26
        }
        hash = (hash * BASE + value) % MAX_ID;
    }
    return hash;
}

unsigned int hash_timestamp(const char *timestamp)
{
    // Extract components from "YYYY-MM-DDTHH:MM:SS"
    int year, month, day, hour, minute, second;
    sscanf(timestamp, "%4d-%2d-%2dT%2d:%2d:%2d", &year, &month, &day, &hour, &minute, &second);

    // Convert the components into a single integer
    unsigned long long hash_value = 0;
    hash_value += (year - 2000) * 518400; // Year: offset by 2000 to save space
    hash_value += month * 43200;          // Month
    hash_value += day * 1440;             // Day
    hash_value += hour * 60;              // Hour
    hash_value += minute;                 // Minute

    // Use modulo to fit the value within the range [0, MAX_ID]
    return (unsigned int)(hash_value % MAX_ID);
}

void generate_test_data(int num_entries)
{
    const char charset[] = "abcdefghijklmnopqrstuvwxyz";
    for (int i = 0; i < num_entries; i++)
    {
        // Generate a random 5-letter name
        for (int j = 0; j < 5; j++)
        {
            entries[i].name[j] = charset[rand() % 26];
        }
        entries[i].name[5] = '\0'; // Null-terminate the string

        // Random ID between 0 and MAX_ID
        entries[i].id = rand() % (MAX_ID + 1);

        // Random timestamp in "YYYY-MM-DDTHH:MM:SS" format
        snprintf(entries[i].timestamp, 20, "2023-%02d-%02dT%02d:%02d:00",
                 rand() % 12 + 1, // Random month (1 to 12)
                 rand() % 28 + 1, // Random day (1 to 28)
                 rand() % 24,     // Random hour (0 to 23)
                 rand() % 60);    // Random minute (0 to 59)
    }
    entry_count = num_entries;
}

void parse_input()
{
    scanf("%d", &entry_count);
    for (int i = 0; i < entry_count; i++)
    {
        scanf("%s %d %s", entries[i].name, &entries[i].id, entries[i].timestamp);
    }
    char sort[10];
    scanf("%s", sort);
    if (strcmp(sort, "ID") == 0)
    {
        sort_criteria = 1;
    }
    if (strcmp(sort, "Name") == 0)
    {
        sort_criteria = 0;
    }
    if (strcmp(sort, "Timestamp") == 0)
    {
        sort_criteria = 2;
    }
}

void print_entries()
{
    if (sort_criteria == 0)
    {
        printf("Name\n");
    }
    if (sort_criteria == 1)
    {
        printf("ID\n");
    }
    if (sort_criteria == 2)
    {
        printf("Timestamp\n");
    }
    for (int i = 0; i < entry_count; i++)
    {
        printf("%s %d %s\n", entries[i].name, entries[i].id, entries[i].timestamp);
    }
}
// ==================== COUNT SORT ====================

void *count_elements(void *arg)
{
    ThreadArgs *args = (ThreadArgs *)arg;
    int *local_count = calloc(MAX_ID + 1, sizeof(int));

    // Count elements based on the selected criteria
    for (int i = args->start_idx; i < args->end_idx; i++)
    {
        int index;
        if (sort_criteria == 0) // Sort by Name
        {
            index = hash_string(entries[i].name); // Using ASCII value of first character
        }
        else if (sort_criteria == 1) // Sort by ID
        {
            index = entries[i].id;
        }
        else // Sort by Timestamp
        {
            index = hash_timestamp(entries[i].timestamp); // Using ASCII value of first character in timestamp
        }
        local_count[index]++;
    }

    // Lock the mutex to safely update the global count array
    pthread_mutex_lock(&lock);
    for (int i = 0; i <= MAX_ID; i++)
    {
        counting_array[i] += local_count[i];
    }
    pthread_mutex_unlock(&lock);

    free(local_count);
    pthread_exit(NULL);
}

void calculate_prefix_sum()
{
    prefix_sum[0] = counting_array[0];
    for (int i = 1; i <= MAX_ID; i++)
    {
        prefix_sum[i] = prefix_sum[i - 1] + counting_array[i];
    }
}

void *place_elements(void *arg)
{
    ThreadArgs *args = (ThreadArgs *)arg;

    for (int i = args->start_idx; i < args->end_idx; i++)
    {
        int index;
        if (sort_criteria == 0) // Sort by Name
        {
            index = hash_string(entries[i].name);
        }
        else if (sort_criteria == 1) // Sort by ID
        {
            index = entries[i].id;
        }
        else // Sort by Timestamp
        {
            index = hash_timestamp(entries[i].timestamp);
        }

        int pos;
        pthread_mutex_lock(&lock);
        pos = --prefix_sum[index];
        pthread_mutex_unlock(&lock);

        sorted_entries[pos] = entries[i];
    }
    pthread_exit(NULL);
}

void parallel_count_sort()
{
    pthread_t threads[MAX_THREADS];
    ThreadArgs thread_args[MAX_THREADS];
    int part_size = entry_count / num_threads;

    // Step 1: Count elements in parallel
    for (int i = 0; i < num_threads; i++)
    {
        thread_args[i].start_idx = i * part_size;
        thread_args[i].end_idx = (i == num_threads - 1) ? entry_count : (i + 1) * part_size;
        pthread_create(&threads[i], NULL, count_elements, &thread_args[i]);
    }

    for (int i = 0; i < num_threads; i++)
    {
        pthread_join(threads[i], NULL);
    }

    // Step 2: Calculate prefix sum (cumulative count array)
    calculate_prefix_sum();

    // Step 3: Place elements in sorted order using multiple threads
    for (int i = 0; i < num_threads; i++)
    {
        pthread_create(&threads[i], NULL, place_elements, &thread_args[i]);
    }

    for (int i = 0; i < num_threads; i++)
    {
        pthread_join(threads[i], NULL);
    }

    // Copy the sorted entries back to the original array
    for (int i = 0; i < entry_count; i++)
    {
        entries[i] = sorted_entries[i];
    }
}

// ==================== MERGE SORT ====================

void merge(Entry arr[], int l, int m, int r)
{
    int n1 = m - l + 1, n2 = r - m;
    Entry L[n1], R[n2];

    for (int i = 0; i < n1; i++)
        L[i] = arr[l + i];
    for (int i = 0; i < n2; i++)
        R[i] = arr[m + 1 + i];

    int i = 0, j = 0, k = l;
    while (i < n1 && j < n2)
    {
        if (compare_entries(&L[i], &R[j]) <= 0)
            arr[k++] = L[i++];
        else
            arr[k++] = R[j++];
    }

    while (i < n1)
        arr[k++] = L[i++];
    while (j < n2)
        arr[k++] = R[j++];
}

void merge_sort_recursive(Entry arr[], int l, int r)
{
    if (l < r)
    {
        int m = l + (r - l) / 2;
        merge_sort_recursive(arr, l, m);
        merge_sort_recursive(arr, m + 1, r);
        merge(arr, l, m, r);
    }
}

void *merge_sort_thread(void *arg)
{
    ThreadData *data = (ThreadData *)arg;
    merge_sort_recursive(entries, data->start, data->end - 1);
    pthread_exit(NULL);
}

void parallel_merge_sort()
{
    pthread_t threads[MAX_THREADS];
    ThreadData thread_data[MAX_THREADS];
    int part_size = entry_count / num_threads;

    for (int i = 0; i < num_threads; i++)
    {
        thread_data[i].start = i * part_size;
        thread_data[i].end = (i == num_threads - 1) ? entry_count : (i + 1) * part_size;
        pthread_create(&threads[i], NULL, merge_sort_thread, &thread_data[i]);
    }

    for (int i = 0; i < num_threads; i++)
    {
        pthread_join(threads[i], NULL);
    }

    for (int i = 1; i < num_threads; i++)
    {
        merge(entries, 0, thread_data[i - 1].end - 1, thread_data[i].end - 1);
    }
}

// ==================== MAIN FUNCTION ====================

int main()
{
    num_threads = 16;
    pthread_mutex_init(&lock, NULL);
    parse_input();
    // generate_test_data(5);
    clock_t start, end;

    if (entry_count < THRESHOLD)
    {
        start = clock();
        parallel_count_sort();
        end = clock();
        printf("Parallel Count Sort Time: %.4f seconds\n", get_time_in_seconds(start, end));
    }
    else
    {
        start = clock();
        parallel_merge_sort();
        end = clock();
        printf("Parallel Merge Sort Time: %.4f seconds\n", get_time_in_seconds(start, end));
    }

    pthread_mutex_destroy(&lock);
    print_entries();
    return 0;
}

double get_time_in_seconds(clock_t start, clock_t end)
{
    return ((double)(end - start)) / CLOCKS_PER_SEC;
}
