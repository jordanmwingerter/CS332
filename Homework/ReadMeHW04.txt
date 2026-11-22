A C program that applies process management, inter-process communication (IPC),
synchronization, file operations, and signals. The program creates
a parent process that generates random numbers using ten producer threads and sends 
them through a pipe. A child process receives the data using consumer threads,
calculates sums, computes an average, and writes the results to an output file.


Requirements

- GCC compiler

## Compile

gcc -Wall -Wextra -pthread hw04.c -o hw04 or make


## Run

./hw04 or make run

Example

During execution, producer threads generate random numbers and write them to a pipe.

```text
Starting producer threads...
Starting consumer threads...

Producer 0: 20% complete
Producer 0: 40% complete
Producer 0: 60% complete

Consumer 3 sum = 116035
Consumer 7 sum = 120849

Producer 0 finished
...
All producers complete.
All consumers complete.
```

After execution, the file output.txt is generated:

text
------------------------------
Consumer Results
------------------------------
Total Sum: 2376548
Average Consumer Sum: 118827.40

How It Works

The program creates a pipe that communicaes between a parent and 
child process. The parent process creates ten producer threads, which generate 500 
unique random integers between 0 and 1000 and writes them to the pipe.
Mutexes and semaphores are used to synchronize access to the pipe.
The child process creates twenty consumer threads. Each consumer thread 
reads 250 integers from the pipe and calculates a local sum. After all
consumer threads complete execution, the child process calculates
the average of all consumer sums. The final results are written to
output.txt through standard output redirection.