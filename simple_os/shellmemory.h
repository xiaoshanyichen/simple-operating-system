#ifndef SHELLMEMORY_H
#define SHELLMEMORY_H

#define MEM_SIZE 1000  // Adjust as needed
#define MAX_SCRIPTS 1000
#define MAX_LINE_LENGTH 100
#include "pcb.h"

#define FRAME_SIZE 3

extern int memoryIndex;  // Declare memoryIndex as external
extern char *scriptMemory[MAX_SCRIPTS];

// Declare the functions and structures for shell memory and scripts
int loadScriptIntoMemory(const char *filename);
char *getScriptLine(int index);

// Other function declarations...
void mem_init();
void mem_set_value(char *var_in, char *value_in);
char *mem_get_value(char *var_in);
char *getLineFromPCB(struct PCB *pcb);
int loadScript(const char *filename, struct PCB *pcb);

void handlePageFault(struct PCB *pcb, int pageNumber);
void initializeFrameStore();
void accessFrame(int frameNumber);

#endif
