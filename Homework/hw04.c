#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <process.h>
#include <io.h>
#include <fcntl.h>
#include <time.h>

#define NUM_PRODUCERS 10
#define NUM_CONSUMERS 20
#define NUM_PER_PRODUCER 500
#define NUM_PER_CONSUMER 250
#define MAX_NUMBER 1000

int pipe_fd[2];

CRITICAL_SECTION write_mutex;
CRITICAL_SECTION read_mutex;
CRITICAL_SECTION print_mutex;

long long consumer_sums[NUM_CONSUMERS];

typedef struct
{
    int id;
} ProducerArgs;

typedef struct
{
    int id;
    int read_fd;
} ConsumerArgs;

int write_all(int fd, const void *buf, size_t size)
{
    const char *p = (const char *)buf;
    size_t total = 0;

    while (total < size)
    {
        int n = _write(fd,
                       p + total,
                       (unsigned int)(size - total));

        if (n <= 0)
            return -1;

        total += n;
    }

    return 0;
}

int read_all(int fd, void *buf, size_t size)
{
    char *p = (char *)buf;
    size_t total = 0;

    while (total < size)
    {
        int n = _read(fd,
                      p + total,
                      (unsigned int)(size - total));

        if (n == 0)
            return 0;

        if (n < 0)
            return -1;

        total += n;
    }

    return 1;
}

DWORD WINAPI producer(LPVOID arg)
{
    ProducerArgs *info = (ProducerArgs *)arg;

    unsigned int seed =
        (unsigned int)time(NULL) +
        (info->id * 7919);

    int pool[MAX_NUMBER + 1];

    for (int i = 0; i <= MAX_NUMBER; i++)
        pool[i] = i;

    for (int i = 0; i < NUM_PER_PRODUCER; i++)
    {
        seed = seed * 1103515245 + 12345;

        int j =
            i + (seed % ((MAX_NUMBER + 1) - i));

        int temp = pool[i];
        pool[i] = pool[j];
        pool[j] = temp;

        EnterCriticalSection(&write_mutex);

        write_all(pipe_fd[1],
                  &pool[i],
                  sizeof(int));

        LeaveCriticalSection(&write_mutex);

        if ((i + 1) % 100 == 0)
        {
            EnterCriticalSection(&print_mutex);

            printf("Producer %d: %d%% complete\n",
                   info->id,
                   ((i + 1) * 100) /
                       NUM_PER_PRODUCER);

            LeaveCriticalSection(&print_mutex);
        }
    }

    EnterCriticalSection(&print_mutex);

    printf("Producer %d finished\n",
           info->id);

    LeaveCriticalSection(&print_mutex);

    return 0;
}

DWORD WINAPI consumer(LPVOID arg)
{
    ConsumerArgs *info =
        (ConsumerArgs *)arg;

    long long sum = 0;

    for (int i = 0;
         i < NUM_PER_CONSUMER;
         i++)
    {
        int value;

        EnterCriticalSection(&read_mutex);

        int status =
            read_all(info->read_fd,
                     &value,
                     sizeof(int));

        LeaveCriticalSection(&read_mutex);

        if (status != 1)
            break;

        sum += value;
    }

    consumer_sums[info->id] = sum;

    EnterCriticalSection(&print_mutex);

    printf("Consumer %d sum = %lld\n",
           info->id,
           sum);

    LeaveCriticalSection(&print_mutex);

    return 0;
}

int main(void)
{
    HANDLE producers[NUM_PRODUCERS];
    HANDLE consumers[NUM_CONSUMERS];

    ProducerArgs pargs[NUM_PRODUCERS];
    ConsumerArgs cargs[NUM_CONSUMERS];

    if (_pipe(pipe_fd,
              4096,
              _O_BINARY) == -1)
    {
        printf("Pipe creation failed\n");
        return 1;
    }

    InitializeCriticalSection(&write_mutex);
    InitializeCriticalSection(&read_mutex);
    InitializeCriticalSection(&print_mutex);

    printf("Starting consumer threads...\n");

    for (int i = 0;
         i < NUM_CONSUMERS;
         i++)
    {
        cargs[i].id = i;
        cargs[i].read_fd = pipe_fd[0];

        consumers[i] =
            CreateThread(
                NULL,
                0,
                consumer,
                &cargs[i],
                0,
                NULL);
    }

    printf("Starting producer threads...\n");

    for (int i = 0;
         i < NUM_PRODUCERS;
         i++)
    {
        pargs[i].id = i;

        producers[i] =
            CreateThread(
                NULL,
                0,
                producer,
                &pargs[i],
                0,
                NULL);
    }

    for (int i = 0;
         i < NUM_PRODUCERS;
         i++)
    {
        WaitForSingleObject(
            producers[i],
            INFINITE);

        CloseHandle(
            producers[i]);
    }

    printf("\nAll producers complete.\n");

    _close(pipe_fd[1]);

    for (int i = 0;
         i < NUM_CONSUMERS;
         i++)
    {
        WaitForSingleObject(
            consumers[i],
            INFINITE);

        CloseHandle(
            consumers[i]);
    }

    long long total = 0;

    for (int i = 0;
         i < NUM_CONSUMERS;
         i++)
    {
        total += consumer_sums[i];
    }

    double average =
        (double)total /
        NUM_CONSUMERS;

    FILE *output =
        fopen("output.txt", "w");

    if (output != NULL)
    {
        fprintf(output,
                "==============================\n");
        fprintf(output,
                "Consumer Results\n");
        fprintf(output,
                "==============================\n");
        fprintf(output,
                "Total Sum: %lld\n",
                total);
        fprintf(output,
                "Average Consumer Sum: %.2f\n",
                average);

        fclose(output);
    }

    _close(pipe_fd[0]);

    DeleteCriticalSection(
        &write_mutex);
    DeleteCriticalSection(
        &read_mutex);
    DeleteCriticalSection(
        &print_mutex);

    printf("Results written to output.txt\n");

    return 0;
}
