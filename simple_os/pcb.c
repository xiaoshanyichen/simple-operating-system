// pcb.c
#include "pcb.h"
#include <stdlib.h>
#include "shellmemory.h"

// Global PID counter
static int pidCounter = 0;

// Create a new PCB
struct PCB* createPCB(int start, int length) {
    struct PCB *pcb = (struct PCB*) malloc(sizeof(struct PCB));
    pcb->pid = pidCounter++;  // Assign unique PID
    pcb->start = start;       // Start index of the script in script memory
    pcb->length = length;     // Total number of lines in the script
    pcb->pc = 0;              // Program counter, starting at 0
    pcb->jobLengthScore = length;
    pcb->next = NULL;

    // Initialize paging related information
    int totalPages = (length + FRAME_SIZE - 1) / FRAME_SIZE; // Calculate total number of pages
    pcb->pages_max = totalPages;
    pcb->pages_loaded = 0;
    pcb->pageTable = (int *)malloc(sizeof(int) * totalPages);
    for (int i = 0; i < totalPages; i++) {
        pcb->pageTable[i] = -1;  // -1 indicates the page is not loaded
    }

    return pcb;
}

// Destroy PCB, free memory
void destroyPCB(struct PCB *pcb) {
    if (pcb != NULL) {
        if (pcb->pageTable != NULL) {
            free(pcb->pageTable);
            pcb->pageTable = NULL;
        }
        free(pcb);
    }
}
