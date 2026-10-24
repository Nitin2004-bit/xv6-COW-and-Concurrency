#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

#define MAX_FILES 10000
#define MAX_USERS 10000

typedef struct
{
    int user_id;
    int file_id;
    char operation[10];
    int request_time;
} Request;

int r, w, d, n, c, T; // READ, WRITE, DELETE times; file count; concurrency; patience
Request requests[MAX_USERS];
int request_count = 0;

// Shared resources
pthread_mutex_t read_mutexes[MAX_FILES];
pthread_mutex_t write_mutexes[MAX_FILES];
pthread_mutex_t delete_mutexes[MAX_FILES];

sem_t file_semaphores[MAX_FILES];
bool file_deleted[MAX_FILES] = {false};

// ANSI Color Codes
#define YELLOW "\033[1;33m"
#define PINK "\033[1;35m"
#define WHITE "\033[1;37m"
#define GREEN "\033[1;32m"
#define RED "\033[1;31m"
#define RESET "\033[0m"

void *process_request(void *arg)
{
    Request *req = (Request *)arg;
    sleep(req->request_time); // Simulate arrival time

    printf(YELLOW "User %d has made request for %s on file %d at %d seconds\n" RESET, req->user_id, req->operation, req->file_id, req->request_time);
    sleep(1);
    if (req->file_id > n)
    {
        printf(RED "Request Cancelled for User %d as there is no such file number\n" RESET, req->user_id);
        return NULL;
    }
    int file_id = req->file_id - 1;
    if (file_deleted[file_id])
    {
        printf(WHITE "LAZY has declined the request of User %d at %d seconds because an invalid/deleted file was requested.\n" RESET, req->user_id, req->request_time);
        return NULL;
    }

    int start_time = req->request_time; // LAZY wakes up after 1 second delay
    if (strcmp(req->operation, "READ") == 0)
    {
        struct timespec timeout;
        clock_gettime(CLOCK_REALTIME, &timeout);
        timeout.tv_sec += T - 1;

        struct timespec start_lock, end_lock;

        // Capture the time before attempting to lock the mutex
        clock_gettime(CLOCK_REALTIME, &start_lock);

        int s;

        if (sem_timedwait(&file_semaphores[file_id], &timeout) == 0)
        {

            clock_gettime(CLOCK_REALTIME, &end_lock);

            // Calculate the time spent waiting for the mutex
            long seconds_spent = end_lock.tv_sec - start_lock.tv_sec;
            long nanoseconds_spent = end_lock.tv_nsec - start_lock.tv_nsec;
            if (nanoseconds_spent < 0)
            {
                seconds_spent -= 1;
                nanoseconds_spent += 1000000000L;
            }
            int time_spent = seconds_spent + nanoseconds_spent / 1e9;

            pthread_mutex_unlock(&read_mutexes[file_id]);
            pthread_mutex_lock(&read_mutexes[file_id]);
            start_time += 1 + time_spent;
            if (start_time >= req->request_time + T)
            {
                printf(RED "User %d canceled the request due to no response at %d seconds\n" RESET, req->user_id, req->request_time + T);
                sem_post(&file_semaphores[file_id]);
                pthread_mutex_unlock(&read_mutexes[file_id]);
                return NULL;
            }
            printf(PINK "LAZY has taken up the request of User %d at %d seconds\n" RESET, req->user_id, start_time);
            sleep(r); // Simulate read time
            int finish_time = start_time + r;
            printf(GREEN "The request for User %d was completed at %d seconds\n" RESET, req->user_id, finish_time);
            sem_post(&file_semaphores[file_id]);
            pthread_mutex_unlock(&read_mutexes[file_id]);
        }
        else
        {
            printf(RED "User %d canceled the request due to no response at %d seconds\n" RESET, req->user_id, start_time + T);
        }
    }
    else if (strcmp(req->operation, "WRITE") == 0)
    {
        struct timespec timeout;
        clock_gettime(CLOCK_REALTIME, &timeout);
        timeout.tv_sec += T - 1;

        struct timespec start_lock, end_lock;

        // Capture the time before attempting to lock the mutex
        clock_gettime(CLOCK_REALTIME, &start_lock);

        // Attempt to lock the mutex with a timeout
        if (pthread_mutex_timedlock(&write_mutexes[file_id], &timeout) != 0 ||
            pthread_mutex_timedlock(&delete_mutexes[file_id], &timeout) != 0)
        {
            printf(RED "User %d canceled the request due to file %d being locked at %d seconds\n" RESET, req->user_id, file_id + 1, start_time + T);
            return NULL;
        }

        // Capture the time after successfully locking the mutex

        if (sem_timedwait(&file_semaphores[file_id], &timeout) == 0)
        {
            clock_gettime(CLOCK_REALTIME, &end_lock);

            // Calculate the time spent waiting for the mutex
            long seconds_spent = end_lock.tv_sec - start_lock.tv_sec;
            long nanoseconds_spent = end_lock.tv_nsec - start_lock.tv_nsec;
            if (nanoseconds_spent < 0)
            {
                seconds_spent -= 1;
                nanoseconds_spent += 1000000000L;
            }
            int time_spent = seconds_spent + nanoseconds_spent / 1e9;

            pthread_mutex_unlock(&delete_mutexes[file_id]);
            start_time += 1 + time_spent;
            if (start_time >= req->request_time + T)
            {
                printf(RED "User %d canceled the request due to no response at %d seconds\n" RESET, req->user_id, req->request_time + T);
                sem_post(&file_semaphores[file_id]);
                return NULL;
            }
            printf(PINK "LAZY has taken up the request of User %d at %d seconds\n" RESET, req->user_id, start_time);
            sleep(w); // Simulate write time
            int finish_time = start_time + w;
            printf(GREEN "The request for User %d was completed at %d seconds\n" RESET, req->user_id, finish_time);
            sem_post(&file_semaphores[file_id]);
        }
        else
        {
            printf(RED "User %d canceled the request due to no response at %d seconds\n" RESET, req->user_id, start_time + T);
        }
        pthread_mutex_unlock(&write_mutexes[file_id]);
    }
    else if (strcmp(req->operation, "DELETE") == 0)
    {
        struct timespec timeout;
        clock_gettime(CLOCK_REALTIME, &timeout);
        timeout.tv_sec += T - 1;

        struct timespec start_lock, end_lock;

        // Capture the time before attempting to lock the mutex
        clock_gettime(CLOCK_REALTIME, &start_lock);

        // Attempt to lock the mutex with a timeout
        if (pthread_mutex_timedlock(&write_mutexes[file_id], &timeout) != 0 || pthread_mutex_timedlock(&read_mutexes[file_id], &timeout) != 0 || pthread_mutex_timedlock(&delete_mutexes[file_id], &timeout) != 0)
        {
            printf(RED "User %d canceled the request due to file %d being locked at %d seconds\n" RESET, req->user_id, file_id + 1, start_time + T);
            return NULL;
        }

        if (sem_timedwait(&file_semaphores[file_id], &timeout) == 0)
        {
            clock_gettime(CLOCK_REALTIME, &end_lock);

            // Calculate the time spent waiting for the mutex
            long seconds_spent = end_lock.tv_sec - start_lock.tv_sec;
            long nanoseconds_spent = end_lock.tv_nsec - start_lock.tv_nsec;
            if (nanoseconds_spent < 0)
            {
                seconds_spent -= 1;
                nanoseconds_spent += 1000000000L;
            }

            int time_spent = seconds_spent + nanoseconds_spent / 1e9;
            pthread_mutex_unlock(&read_mutexes[file_id]);
            pthread_mutex_unlock(&write_mutexes[file_id]);
            start_time += 1 + time_spent;
            if (start_time >= req->request_time + T)
            {
                printf(RED "User %d canceled the request due to no response at %d seconds\n" RESET, req->user_id, req->request_time + T);
                sem_post(&file_semaphores[file_id]);
                return NULL;
            }
            printf(PINK "LAZY has taken up the request of User %d at %d seconds\n" RESET, req->user_id, start_time);
            file_deleted[file_id] = true;
            sleep(d); // Simulate delete time
            int finish_time = start_time + d;
            printf(GREEN "The request for User %d was completed at %d seconds\n" RESET, req->user_id, finish_time);
            sem_post(&file_semaphores[file_id]);
        }
        else
        {
            printf(RED "User %d canceled the request due to no response at %d seconds\n" RESET, req->user_id, start_time + T);
        }
        pthread_mutex_unlock(&delete_mutexes[file_id]);
    }
    return NULL;
}

int main()
{
    // Input simulation parameters
    scanf("%d %d %d", &r, &w, &d);
    scanf("%d %d %d", &n, &c, &T);

    // Initialize semaphores and mutexes for files
    for (int i = 0; i < n; i++)
    {
        pthread_mutex_init(&write_mutexes[i], NULL);
        pthread_mutex_init(&read_mutexes[i], NULL);
        pthread_mutex_init(&delete_mutexes[i], NULL);
        sem_init(&file_semaphores[i], 0, c); // Initialize with max concurrency count
    }

    // Read requests
    while (1)
    {
        Request req;
        if (scanf("%d %d %s %d", &req.user_id, &req.file_id, req.operation, &req.request_time) != 4)
            break;
        requests[request_count++] = req;
    }

    printf("LAZY has woken up!\n");

    // Create threads for each request
    pthread_t threads[request_count];
    for (int i = 0; i < request_count; i++)
    {
        pthread_create(&threads[i], NULL, process_request, (void *)&requests[i]);
    }

    // Wait for all threads to complete
    for (int i = 0; i < request_count; i++)
    {
        pthread_join(threads[i], NULL);
    }

    printf("LAZY has no more pending requests and is going back to sleep!\n");

    // Clean up resources
    for (int i = 0; i < n; i++)
    {
        pthread_mutex_destroy(&write_mutexes[i]);
        pthread_mutex_destroy(&read_mutexes[i]);
        pthread_mutex_destroy(&delete_mutexes[i]);
        sem_destroy(&file_semaphores[i]);
    }

    return 0;
}
