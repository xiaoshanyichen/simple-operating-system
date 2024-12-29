int interpreter(char *command_args[], int args_size);
int help();
int quit();
#include <pthread.h>
#define NUM_THREADS 2

extern pthread_t workerThreads[NUM_THREADS];  // Array to hold thread IDs
extern pthread_mutex_t readyQueueMutex;  // Mutex to control access to the ready queue
extern int multithreadEnabled;  // Flag to indicate if multithreading is enabled
extern int quitRequested;


