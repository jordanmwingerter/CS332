#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <sys/wait.h>
#include <semaphore.h>

#define NUM_PRODUCERS 10
#define NUM_CONSUMERS 20
#define NUM_PER_PRODUCER 500
#define NUM_PER_CONSUMER 250
#define MAX_NUMBER 1000

int pipe_fd[2];

pthread_mutex_t write_mutex;
pthread_mutex_t read_mutex;
pthread_mutex_t print_mutex;
sem_t pipe_sem;

long long consumer_sums[NUM_CONSUMERS];

typedef struct{
    int id;
} ProducerArgs;

typedef struct{
    int id;
    int read_fd;
} ConsumerArgs;

int write_all(int fd, const void *buf, size_t size){
    const char *p = (const char *)buf;
    size_t total = 0;

    while (total < size){
        int n = write(fd,
                       p + total,
                       (unsigned int)(size - total));

        if (n <= 0)
            return -1;

        total += n;
    }

    return 0;
}

int read_all(int fd, void *buf, size_t size){
    char *p = (char *)buf;
    size_t total = 0;

    while (total < size){
        int n = read(fd,
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

void *producer(void *arg){
    ProducerArgs *info = (ProducerArgs *)arg;

    unsigned int seed =
        (unsigned int)time(NULL) +
        (info->id * 7919);

    int pool[MAX_NUMBER + 1];

    for (int i = 0; i <= MAX_NUMBER; i++)
        pool[i] = i;

    for (int i = 0; i < NUM_PER_PRODUCER; i++){
        seed = seed * 1103515245 + 12345;

        int j = i + (seed % ((MAX_NUMBER + 1) - i));

        int temp = pool[i];
        pool[i] = pool[j];
        pool[j] = temp;
        
        sem_wait(&pipe_sem);

        pthread_mutex_lock(&write_mutex);

        write_all(pipe_fd[1],
            &pool[i],
            sizeof(int));

        pthread_mutex_unlock(&write_mutex);

        sem_post(&pipe_sem);

        

        if ((i + 1) % 100 == 0)
        {
            pthread_mutex_lock(&print_mutex);

            printf("Producer %d: %d%% complete\n",
                   info->id,
                   ((i + 1) * 100) /
                       NUM_PER_PRODUCER);

            pthread_mutex_unlock(&print_mutex);
        }
    }

    pthread_mutex_lock(&print_mutex);


    printf("Producer %d finished\n",
           info->id);

    pthread_mutex_unlock(&print_mutex);

    return NULL;
}

void *consumer(void *arg){
    ConsumerArgs *info =
        (ConsumerArgs *)arg;

    long long sum = 0;

    for (int i = 0;
         i < NUM_PER_CONSUMER;
         i++)
    {
        int value;

        sem_wait(&pipe_sem);

        pthread_mutex_lock(&read_mutex);

        int status = 
            read_all(info->read_fd,
                    &value,
                    sizeof(int));

        pthread_mutex_unlock(&read_mutex);

        sem_post(&pipe_sem);
        
        
        if (status != 1)
            break;

        sum += value;
    }
    consumer_sums[info->id] = sum;

    pthread_mutex_lock(&print_mutex);


    printf("Consumer %d sum = %lld\n",
           info->id,
           sum);

    pthread_mutex_unlock(&print_mutex);

    return NULL;
}

int main(void){
    pthread_t producers[NUM_PRODUCERS];
    pthread_t consumers[NUM_CONSUMERS];

    ProducerArgs pargs[NUM_PRODUCERS];
    ConsumerArgs cargs[NUM_CONSUMERS];

    if (pipe(pipe_fd) == -1){
        printf("Pipe creation failed\n");
        return 1;
    }
    pid_t pid = fork();

    if (pid < 0){
        perror("fork");
        return 1;
    }   
    

    pthread_mutex_init(&write_mutex, NULL);
    pthread_mutex_init(&read_mutex, NULL);
    pthread_mutex_init(&print_mutex, NULL);
    sem_init(&pipe_sem, 0, 1);

    if (pid > 0){
        close(pipe_fd[0]); /* close read end */

        printf("Starting producer threads...\n");

        for (int i = 0; i < NUM_PRODUCERS; i++){
            pargs[i].id = i;

        pthread_create(
            &producers[i],
            NULL,
            producer,
            &pargs[i]);
    }

    for (int i = 0; i < NUM_PRODUCERS; i++){
        pthread_join(
            producers[i],
            NULL);
    }

    printf("\nAll producers complete.\n");

    close(pipe_fd[1]);

    wait(NULL);
    sem_destroy(&pipe_sem);

    return 0;
    }else{
        close(pipe_fd[1]);

        printf("Starting consumer threads...\n");

        for (int i = 0; i < NUM_CONSUMERS; i++){
            cargs[i].id = i;
            cargs[i].read_fd = pipe_fd[0];

            pthread_create(
                &consumers[i],
                NULL,
                consumer,
                &cargs[i]);
        }

    

    

    for (int i = 0;
         i < NUM_CONSUMERS;
         i++)
    {
        pthread_join(
            consumers[i],
            NULL);
    }
    printf("\nAll consumers complete.\n");

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

    freopen(
        "output.txt",
        "w",
        stdout);

    printf("==============================\n"); 
    printf("Consumer Results\n");
    printf("==============================\n"); 
    printf("Total Sum: %lld\n", total);
    printf("Average Consumer Sum: %.2f\n",
        average);

    close(pipe_fd[0]);

    pthread_mutex_destroy(
        &write_mutex);
    pthread_mutex_destroy(
        &read_mutex);
    pthread_mutex_destroy(
        &print_mutex);

    printf("Results written to output.txt\n");

    return 0;
    }

}
