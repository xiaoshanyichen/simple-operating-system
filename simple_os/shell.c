// shell.c
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h> // isspace
#include <string.h>
#include <unistd.h> // isatty
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include "shell.h"
#include "interpreter.h"
#include "shellmemory.h"
#include <pthread.h>
#include <sys/select.h>

#define MAX_USER_INPUT 1000

// Declare functions
int parseInput(char ui[]);
void clearBackingStore();
void removeBackingStore();

// Main function
int main(int argc, char *argv[]) {
    printf("Frame Store Size = %d; Variable Store Size = %d\n", FRAME_STORE_SIZE, VARIABLE_STORE_SIZE);

    char prompt = '$';  				// Shell prompt
    char userInput[MAX_USER_INPUT];		// Store user input
    int batch_mode = !isatty(STDIN_FILENO);
    int errorCode = 0;					// Error code, 0 indicates no error

    // Initialize user input
    for (int i = 0; i < MAX_USER_INPUT; i++) {
        userInput[i] = '\0';
    }

    // Initialize Shell memory
    mem_init();

   // initializeFrameStore();

    // Initialize backing store
    struct stat st = {0};
    if (stat("backing_store", &st) == -1) {
        mkdir("backing_store", 0777);
    } else {
        // If backing store exists, clear its contents
        clearBackingStore();
    }

    while(1) {
        if (!batch_mode) {
            printf("%c ", prompt);
        }
        fgets(userInput, MAX_USER_INPUT-1, stdin);
        errorCode = parseInput(userInput);
        if (errorCode == -1) exit(99); // Ignore other errors

        if (feof(stdin)) {
            // If end of file is reached, exit Shell
            for (int i = 0; i < NUM_THREADS; i++) {
                pthread_join(workerThreads[i], NULL);
            }
            pthread_mutex_destroy(&readyQueueMutex);
            removeBackingStore();  // Delete backing store
            exit(0);
        }

        memset(userInput, 0, sizeof(userInput));
    }

    return 0;
}

// Clear backing store directory
void clearBackingStore() {
    DIR *dir = opendir("backing_store");
    if (dir) {
        struct dirent *entry;
        char path[256];
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..")) {
                // Ensure truncation cannot exceed buffer size
                snprintf(path, sizeof(path), "backing_store/%.240s", entry->d_name);
                remove(path);
            }
        }
        closedir(dir);
    }
}


// Delete backing store directory
void removeBackingStore() {
    clearBackingStore();
    rmdir("backing_store");
}

// Determine word ending
int wordEnding(char c) {
    return c == '\0' || c == '\n' || isspace(c) || c == ';';
}

// Parse user input
int parseInput(char inp[]) {
    char tmp[200], *words[100];
    int ix = 0, w = 0;
    int wordlen;
    int errorCode = 0;

    while (inp[ix] != '\n' && inp[ix] != '\0' && ix < 1000) {
        // Skip whitespace characters
        for ( ; isspace(inp[ix]) && inp[ix] != '\n' && ix < 1000; ix++);

        // If the next character is a semicolon, execute the parsed command
        if (inp[ix] == ';') break;

        // Extract a word
        for (wordlen = 0; !wordEnding(inp[ix]) && ix < 1000; ix++, wordlen++) {
            tmp[wordlen] = inp[ix];
        }

        if (wordlen > 0) {
            tmp[wordlen] = '\0';
            words[w] = strdup(tmp);
            w++;
            if (inp[ix] == '\0') break;
        } else {
            break;
        }
    }

    errorCode = interpreter(words, w);
    for (size_t i = 0; i < w; ++i) {
        free(words[i]);
    }

    if (inp[ix] == ';') {
        // Handle chained commands
        return parseInput(&inp[ix+1]);
    }
    return errorCode;
}

// Load Shell input as program (supports background execution)
int loadShellInputAsProgram() {
    char shellInput[MAX_USER_INPUT];
    int start = memoryIndex;  // Start index of the program in script memory

    // Read all input lines and store them as a program
    while (fgets(shellInput, MAX_USER_INPUT - 1, stdin) != NULL) {
        // Check if script memory has reached its maximum capacity
        if (memoryIndex >= MAX_SCRIPTS) {
            printf("Error: Script memory is full, cannot store more lines.\n");
            break;
        }

        // Store each line in scriptMemory
        scriptMemory[memoryIndex] = (char *)malloc(strlen(shellInput) + 1);
        if (scriptMemory[memoryIndex] == NULL) {
            printf("Error: Failed to allocate memory for input line.\n");
            break;
        }
        strcpy(scriptMemory[memoryIndex], shellInput);

        // Increment memoryIndex
        memoryIndex++;
    }

    return start;  // Return the start index of the program in script memory
}
