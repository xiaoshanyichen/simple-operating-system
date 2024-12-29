// pcb.h

#ifndef PCB_H
#define PCB_H

struct PCB {
    int pid;              // Process ID
    int start;            // Start index of the script in script memory
    int length;           // Total number of lines in the script
    int pc;               // Program counter, indicating the current executing line
    int jobLengthScore;   // Job length score for scheduling
    struct PCB *next;     // Pointer to the next PCB (for the ready queue)

    int *pageTable;       // Page table, mapping pages to frames
    int pages_max;        // Total number of pages
    int pages_loaded;     // Number of pages loaded
};

struct PCB* createPCB(int start, int length);
void destroyPCB(struct PCB *pcb);

#endif
