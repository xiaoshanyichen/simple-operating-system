// shellmemory.c

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "shellmemory.h"
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "pcb.h"

// Define frame size and storage size
#define MAX_LINE_LENGTH 100  // Maximum length per line
#define MAX_SCRIPTS 1000     // Maximum number of script lines

#define FRAME_SIZE 3
//#define FRAME_STORE_SIZE 300  // Adjust based on actual requirements
//#define VARIABLE_STORE_SIZE 10  // Adjust based on actual requirements

// Calculate total number of frames
#define FRAME_COUNT (FRAME_STORE_SIZE / FRAME_SIZE)

// Variable storage structure
struct memory_struct {
    char *var;
    char *value;
};

// Global variables and data structures
struct memory_struct variableStore[VARIABLE_STORE_SIZE];  // Variable storage area
char *frameStore[FRAME_COUNT * FRAME_SIZE];               // Frame storage area
int frameUsage[FRAME_COUNT];                              // Tracks the last usage time of each frame
int currentTime = 0;                                      // Global time counter for LRU algorithm

// Script memory (for backing store)
char *scriptMemory[MAX_SCRIPTS];  // Stores script lines
int memoryIndex = 0;              // Tracks the next storage position in scriptMemory

// Global PCB linked list head for updating page tables
extern struct PCB *pcbListHead;

// Initialize memory
void mem_init() {
    int i;
    // Initialize variable storage area
    for (i = 0; i < VARIABLE_STORE_SIZE; i++) {
        variableStore[i].var = NULL;
        variableStore[i].value = NULL;
    }

    // Initialize frame storage area
    for (i = 0; i < FRAME_COUNT * FRAME_SIZE; i++) {
        frameStore[i] = NULL;
    }
    for (i = 0; i < FRAME_COUNT; i++) {
        frameUsage[i] = -1;  // Mark frame as unused
    }
    currentTime = 0;

    // Initialize script memory
    for (i = 0; i < MAX_SCRIPTS; i++) {
        scriptMemory[i] = NULL;
    }

    // Other initialization...
}

// Update the last access time of a frame
void accessFrame(int frameNumber) {
    currentTime++;
    frameUsage[frameNumber] = currentTime;  // Update access time
}

// Set the value of a variable
void mem_set_value(char *var_in, char *value_in) {
    int i;

    // Search if the variable already exists
    for (i = 0; i < VARIABLE_STORE_SIZE; i++) {
        if (variableStore[i].var != NULL && strcmp(variableStore[i].var, var_in) == 0) {
            free(variableStore[i].value);  // Free the old value
            variableStore[i].value = strdup(value_in);
            return;
        }
    }

    // If the variable does not exist, find an empty slot
    for (i = 0; i < VARIABLE_STORE_SIZE; i++) {
        if (variableStore[i].var == NULL) {
            variableStore[i].var = strdup(var_in);
            variableStore[i].value = strdup(value_in);
            return;
        }
    }
}

// Get the value of a variable
char *mem_get_value(char *var_in) {
    int i;

    for (i = 0; i < VARIABLE_STORE_SIZE; i++) {
        if (variableStore[i].var != NULL && strcmp(variableStore[i].var, var_in) == 0) {
            return strdup(variableStore[i].value);
        }
    }
    return NULL;
}

// Find a free frame
int findFreeFrame() {
    for (int i = 0; i < FRAME_COUNT; i++) {
        if (frameUsage[i] == -1) {
            return i;  // Return the number of the free frame
        }
    }
    return -1;  // No free frames
}

// Load a page into a frame
void loadPageIntoFrame(int scriptStart, int pageNumber, int frameNumber) {
    int lineNumber = scriptStart + pageNumber * FRAME_SIZE;
    int frameStartIndex = frameNumber * FRAME_SIZE;
    for (int i = 0; i < FRAME_SIZE; i++) {
        int frameIndex = frameStartIndex + i;
        if (lineNumber + i < memoryIndex && scriptMemory[lineNumber + i] != NULL) {
            frameStore[frameIndex] = strdup(scriptMemory[lineNumber + i]);
        } else {
            frameStore[frameIndex] = NULL; // Empty line
        }
    }
    // Do not update frameUsage and currentTime here
}

// Update the page tables of all PCBs
void updatePageTables(int evictedFrameNumber) {
    // Traverse all PCBs and update their page tables
    struct PCB *pcb = pcbListHead;
    while (pcb != NULL) {
        for (int i = 0; i < pcb->pages_max; i++) {
            if (pcb->pageTable[i] == evictedFrameNumber) {
                pcb->pageTable[i] = -1;  // Mark page as not loaded
            }
        }
        pcb = pcb->next;
    }
}

// Evict the least recently used frame
int evictLRUFrame() {
    int lruFrame = -1;
    int minTime = currentTime + 1;  // Initialize to a large value

    // Find the least recently used frame
    for (int i = 0; i < FRAME_COUNT; i++) {
        if (frameUsage[i] == -1) {
            continue;  // Skip unused frames
        }
        if (frameUsage[i] < minTime) {
            minTime = frameUsage[i];
            lruFrame = i;
        }
    }

    if (lruFrame == -1) {
        // No frame to evict; handle error
        printf("Error: No frames to evict.\n");
        exit(1);
    }

    // Print the message indicating eviction
    printf("Page fault! Victim page contents:\n\n");

    // Print and free the victim page contents
    int frameStartIndex = lruFrame * FRAME_SIZE;
    for (int i = 0; i < FRAME_SIZE; i++) {
        int frameIndex = frameStartIndex + i;
        if (frameStore[frameIndex]) {
            printf("%s", frameStore[frameIndex]);
            free(frameStore[frameIndex]);
            frameStore[frameIndex] = NULL;
        }
    }

    printf("\nEnd of victim page contents.\n");

    frameUsage[lruFrame] = -1; // Mark the frame as unused

    // Update all PCBs' page tables
    updatePageTables(lruFrame);

    return lruFrame;
}


// Handle a page fault
void handlePageFault(struct PCB *pcb, int pageNumber) {
    int frameNumber = findFreeFrame();
    if (frameNumber == -1) {
        // No free frame, eviction is needed
        frameNumber = evictLRUFrame();
        // In evictLRUFrame(), the "Page fault! Victim page contents:" message is printed
    } else {
        // Free frame is available
        printf("Page fault!\n");
    }

    // Load the missing page into the selected frame
    loadPageIntoFrame(pcb->start, pageNumber, frameNumber);

    // Update the page table
    pcb->pageTable[pageNumber] = frameNumber;
    pcb->pages_loaded++;

    // Update the frame usage time
    accessFrame(frameNumber);
}


// Get a line of script from the PCB's page table
char *getLineFromPCB(struct PCB *pcb) {
    if (pcb->pc >= pcb->length) {
        return NULL;  // Program has ended
    }

    int pageNumber = pcb->pc / FRAME_SIZE;
    int offset = pcb->pc % FRAME_SIZE;

    int frameNumber = pcb->pageTable[pageNumber];
    if (frameNumber == -1) {
        // Page not loaded
        return NULL;
    }

    int frameIndex = frameNumber * FRAME_SIZE + offset;
    return frameStore[frameIndex];
}



// Load a script into memory (supports paging)
int loadScript(const char *filename, struct PCB *pcb) {
    // Check if the script exists
    char backingStorePath[256];
    snprintf(backingStorePath, sizeof(backingStorePath), "backing_store/%s", filename);

    FILE *sourceFile = fopen(filename, "r");
    if (!sourceFile) {
        printf("Error: Cannot open script file %s\n", filename);
        return -1;
    }

    FILE *destFile = fopen(backingStorePath, "w");
    if (!destFile) {
        printf("Error: Cannot create backing store file %s\n", backingStorePath);
        fclose(sourceFile);
        return -1;
    }

    // Copy the script to the backing store
    char line[MAX_LINE_LENGTH];
    while (fgets(line, sizeof(line), sourceFile)) {
        fputs(line, destFile);
    }
    fclose(sourceFile);
    fclose(destFile);

    // Load the script into script memory
    FILE *file = fopen(backingStorePath, "r");
    if (!file) {
        printf("Error: Cannot open backing store file %s\n", backingStorePath);
        return -1;
    }

    int startIndex = memoryIndex; // Start index of the script in scriptMemory
    int lineCount = 0;
    while (fgets(line, sizeof(line), file)) {
        if (memoryIndex >= MAX_SCRIPTS) {
            printf("Error: Script memory is full\n");
            fclose(file);
            return -1;
        }
        scriptMemory[memoryIndex] = strdup(line); // Store in logical memory
        memoryIndex++;
        lineCount++;
    }
    fclose(file);

    pcb->start = startIndex;
    pcb->length = lineCount;

    // Calculate the number of pages required
    pcb->pages_max = (lineCount + FRAME_SIZE - 1) / FRAME_SIZE;
    pcb->pages_loaded = 0;

    pcb->pageTable = (int *)malloc(sizeof(int) * pcb->pages_max);
    for (int i = 0; i < pcb->pages_max; i++) {
        pcb->pageTable[i] = -1; // Mark all pages as not loaded
    }

    // Load the first two pages (if applicable)
    int pagesToLoad = pcb->pages_max > 2 ? 2 : pcb->pages_max;
    for (int pageNum = 0; pageNum < pagesToLoad; pageNum++) {
        int frameNumber = findFreeFrame();
        if (frameNumber == -1) {
            frameNumber = evictLRUFrame();
        }
        loadPageIntoFrame(pcb->start, pageNum, frameNumber);
        pcb->pageTable[pageNum] = frameNumber;
        pcb->pages_loaded++;
        accessFrame(frameNumber);  // Update frame usage time
    }

    return 0;
}
