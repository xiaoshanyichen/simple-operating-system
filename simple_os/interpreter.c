// interpreter.c

//#define DEBUG 1

#ifdef DEBUG
#define debug(...) fprintf(stderr, __VA_ARGS__)
#else
#define debug(...)
#define NDEBUG
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h> // tolower, isdigit, isalpha
#include <dirent.h> // scandir
#include <unistd.h> // chdir
#include <sys/stat.h> // mkdir
#include <pthread.h>

#include "pcb.h"
#include "scheduler.h"
#include "shellmemory.h"
#include "shell.h"
#include "interpreter.h"

#define MAX_ARGS_SIZE 7
#define NUM_THREADS 2

pthread_t workerThreads[NUM_THREADS];  // Array to hold thread IDs
pthread_mutex_t readyQueueMutex;       // Mutex to control access to the ready queue
int multithreadEnabled = 0;            // Flag to indicate if multithreading is enabled
int quitRequested = 0;                 // Flag to indicate if quit is called in any worker threads

int badcommand() {
    printf("Unknown Command\n");
    return 1;
}

int badcommandTooLong() {
    printf("Bad command: Too many tokens\n");
    return 2;
}

// For run command
int badcommandFileDoesNotExist() {
    printf("Bad command: File not found\n");
    return 3;
}

int badcommandMkdir() {
    printf("Bad command: my_mkdir\n");
    return 4;
}

int badcommandCd() {
    printf("Bad command: my_cd\n");
    return 5;
}

int help();
int quit();
int set(char *var, char *value[], int value_size);
int print(char *var);
int echo(char *tok);
int ls();
int my_mkdir(char *name);
int touch(char *path);
int cd(char *path);
int run(char *script);
int exec(char *prog1, char *prog2, char *prog3, char *policy, int background, int multithread);

// Add definition of str_isalphanum function
int str_isalphanum(char *name) {
    for (char *ptr = name; *ptr != '\0'; ptr++) {
        if (!(isdigit(*ptr) || isalpha(*ptr))) {
            return 0; // Return 0 if a non-alphanumeric character is encountered
        }
    }
    return 1; // Return 1 if the string contains only letters and numbers
}

// Interpret commands and their arguments
int interpreter(char *command_args[], int args_size) {
    int i;
    int background = 0;
    int multithread = 0;

    debug("#args: %d\n", args_size);
#ifdef DEBUG
    for (size_t i = 0; i < args_size; ++i) {
        debug("  %ld: %s\n", i, command_args[i]);
    }
#endif

    if (args_size < 1) {
        return badcommand();
    }
    if (args_size > MAX_ARGS_SIZE) {
        return badcommandTooLong();
    }

    for (i = 0; i < args_size; i++) { // Terminate arguments at a new line
        command_args[i][strcspn(command_args[i], "\r\n")] = 0;
    }

    if (strcmp(command_args[0], "help") == 0){
        if (args_size != 1) return badcommand();
        return help();

    } else if (strcmp(command_args[0], "quit") == 0) {
        if (args_size != 1) return badcommand();
        return quit();

    } else if (strcmp(command_args[0], "set") == 0) {
        if (args_size < 3) return badcommand();
        if (args_size > 7) return badcommand();
        return set(command_args[1], &command_args[2], args_size-2);

    } else if (strcmp(command_args[0], "print") == 0) {
        if (args_size != 2) return badcommand();
        return print(command_args[1]);

    } else if (strcmp(command_args[0], "echo") == 0) {
        if (args_size != 2) return badcommand();
        return echo(command_args[1]);

    } else if (strcmp(command_args[0], "my_ls") == 0) {
        if (args_size != 1) return badcommand();
        return ls();

    } else if (strcmp(command_args[0], "my_mkdir") == 0) {
        if (args_size != 2) return badcommand();
        return my_mkdir(command_args[1]);

    } else if (strcmp(command_args[0], "my_touch") == 0) {
        if (args_size != 2) return badcommand();
        return touch(command_args[1]);

    } else if (strcmp(command_args[0], "my_cd") == 0) {
        if (args_size != 2) return badcommand();
        return cd(command_args[1]);

    } else if (strcmp(command_args[0], "run") == 0) {
        if (args_size != 2) return badcommand();
        return run(command_args[1]);

    } else if (strcmp(command_args[0], "exec") == 0) {
        // Parse exec command, including optional arguments
        if (args_size < 3 || args_size > 7) {
            return badcommand();
        }

        int progCount = 0;
        char *progs[3] = {NULL, NULL, NULL};
        char *policy = NULL;

        // Traverse command arguments
        for (int i = 1; i < args_size; i++) {
            if (strcmp(command_args[i], "MT") == 0) {
                multithread = 1;
            } else if (strcmp(command_args[i], "#") == 0) {
                background = 1;
            } else if (strcmp(command_args[i], "FCFS") == 0 ||
                       strcmp(command_args[i], "SJF") == 0 ||
                       strcmp(command_args[i], "RR") == 0 ||
                       strcmp(command_args[i], "AGING") == 0 ||
                       strcmp(command_args[i], "RR30") == 0) {
                policy = command_args[i];
            } else if (progCount < 3) {
                progs[progCount++] = command_args[i];
            } else {
                return badcommand(); // Too many program arguments
            }
        }

        if (policy == NULL) {
            return badcommand(); // Scheduling policy is required
        }

        // Assign programs
        char *prog1 = progs[0];
        char *prog2 = progs[1];
        char *prog3 = progs[2];

        // Call exec function, passing background and multithread flags
        return exec(prog1, prog2, prog3, policy, background, multithread);
    }
    else return badcommand();
}

int help() {

    // Note that the tab is used here for alignment
    char help_string[] = "COMMAND            DESCRIPTION\n \
help                Displays all the commands\n \
quit                Exits / terminates the shell with “Bye!”\n \
set VAR STRING      Assigns a value to shell memory\n \
print VAR           Displays the STRING assigned to VAR\n \
run SCRIPT.TXT      Executes the file SCRIPT.TXT\n";
    printf("%s\n", help_string);
    return 0;
}

int quit() {
    printf("Bye!\n");

    if (multithreadEnabled) {
        quitRequested = 1;
        return 0;
    }

    // Remove backing store
    removeBackingStore();

    exit(0);
}

int set(char *var, char *value[], int value_size) {
    char buffer[MAX_USER_INPUT];
    char *space = " ";

    strcpy(buffer, value[0]);
    for (size_t i = 1; i < value_size; i++) {
        strcat(buffer, space);
        strcat(buffer, value[i]);
    }

    mem_set_value(var, buffer);

    return 0;
}

int print(char *var) {
    char *value = mem_get_value(var);
    if (value) {
        printf("%s\n", value);
        free(value);
    } else {
        printf("Variable does not exist\n");
    }
    return 0;
}

int echo(char *tok) {
    int must_free = 0;
    if (tok[0] == '$') {
        tok++;
        tok = mem_get_value(tok);
        if (tok == NULL) {
            tok = "";
        } else {
            must_free = 1;
        }
    }
    //printf("Front");
    printf("%s\n", tok);
    //printf("Back");

    if (must_free) free(tok);


    return 0;
}

int ls() {
    struct dirent **namelist;
    int n;

    n = scandir(".", &namelist, NULL, alphasort);
    if (n == -1) {
        perror("my_ls couldn't scan the directory");
        return 0;
    }

    for (size_t i = 0; i < n; ++i) {
        if (namelist[i]->d_name[0] != '.')
            printf("%s\n", namelist[i]->d_name);
        free(namelist[i]);
    }
    free(namelist);

    return 0;
}

int my_mkdir(char *name) {
    int must_free = 0;

    debug("my_mkdir: ->%s<-\n", name);

    if (name[0] == '$') {
        ++name;
        name = mem_get_value(name);
        debug("  lookup: %s\n", name ? name : "(NULL)");
        if (name) {
            must_free = 1;
        }
    }
    if (!name || !str_isalphanum(name)) {
        if (must_free) free(name);
        return badcommandMkdir();
    }

    int result = mkdir(name, 0777);

    if (result) {
        perror("Something went wrong in my_mkdir");
    }

    if (must_free) free(name);
    return 0;
}

int touch(char *path) {
    assert(str_isalphanum(path));
    FILE *f = fopen(path, "a");
    fclose(f);
    return 0;
}

int cd(char *path) {
    assert(str_isalphanum(path));

    int result = chdir(path);
    if (result) {
        return badcommandCd();
    }
    return 0;
}

// Handle "run" command
int run(char *script) {
    // Create a new PCB
    struct PCB *pcb = createPCB(0, 0);

    // Load script into memory with paging support
    int result = loadScript(script, pcb);
    if (result == -1) {
        destroyPCB(pcb);
        return badcommandFileDoesNotExist();
    }

    // Add PCB to ready queue
    enqueue(pcb);

    // Run the scheduler
    runScheduler();

    return 0;
}

int exec(char *prog1, char *prog2, char *prog3, char *policy, int background, int multithread) {
    // Create PCBs for programs
    struct PCB *pcb1 = NULL, *pcb2 = NULL, *pcb3 = NULL;

    if (prog1) {
        pcb1 = createPCB(0, 0);
        int result = loadScript(prog1, pcb1);
        if (result == -1) {
            printf("Error: Could not load %s\n", prog1);
            destroyPCB(pcb1);
            return -1;
        }
    }

    if (prog2) {
        pcb2 = createPCB(0, 0);
        int result = loadScript(prog2, pcb2);
        if (result == -1) {
            printf("Error: Could not load %s\n", prog2);
            destroyPCB(pcb2);
            return -1;
        }
    }

    if (prog3) {
        pcb3 = createPCB(0, 0);
        int result = loadScript(prog3, pcb3);
        if (result == -1) {
            printf("Error: Could not load %s\n", prog3);
            destroyPCB(pcb3);
            return -1;
        }
    }

    // Schedule programs according to policy
    if (strcmp(policy, "FCFS") == 0) {
        if (pcb1) enqueue(pcb1);
        if (pcb2) enqueue(pcb2);
        if (pcb3) enqueue(pcb3);

        runScheduler();  // Run in foreground

    } else if (strcmp(policy, "SJF") == 0) {
        if (pcb1) enqueueSJF(pcb1);  // SJF - Sort based on script length
        if (pcb2) enqueueSJF(pcb2);
        if (pcb3) enqueueSJF(pcb3);

        runScheduler();

    } else if (strcmp(policy, "RR") == 0) {
        if (pcb1) enqueue(pcb1);
        if (pcb2) enqueue(pcb2);
        if (pcb3) enqueue(pcb3);

        if (multithread) {
            if (!multithreadEnabled) {
                multithreadEnabled = 1;
                pthread_mutex_init(&readyQueueMutex, NULL);
                for (int i = 0; i < NUM_THREADS; i++) {
                    pthread_create(&workerThreads[i], NULL, (void *(*)(void *))runSchedulerRR, NULL);
                }
                if (quitRequested) {
                    exit(0);
                }
            }
        }
        else {
            runSchedulerRR();
        }

    } else if (strcmp(policy, "AGING") == 0) {
        if (pcb1) enqueueSJFAging(pcb1);
        if (pcb2) enqueueSJFAging(pcb2);
        if (pcb3) enqueueSJFAging(pcb3);

        runSchedulerSJFwithAging();

    } else if (strcmp(policy, "RR30") == 0) {
        if (pcb1) enqueue(pcb1);
        if (pcb2) enqueue(pcb2);
        if (pcb3) enqueue(pcb3);

        if (multithread) {
            if (!multithreadEnabled) {
                multithreadEnabled = 1;
                pthread_mutex_init(&readyQueueMutex, NULL);
                for (int i = 0; i < NUM_THREADS; i++) {
                    pthread_create(&workerThreads[i], NULL, (void *(*)(void *))runSchedulerRR30, NULL);
                }
                if (quitRequested) {
                    exit(0);
                }
            }
        }
        else {
            runSchedulerRR30();
        }
    }
    return 0;
}
