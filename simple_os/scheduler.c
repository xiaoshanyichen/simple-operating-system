// scheduler.c

#include "scheduler.h"
#include "pcb.h"
#include "shellmemory.h"
#include "interpreter.h"
#include "shell.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

// Head and tail of the ready queue
static struct PCB *readyQueueHead = NULL;
static struct PCB *readyQueueTail = NULL;
void ageReadyQueue(struct PCB *currentProcess);
struct PCB* findLowestScoreJob(struct PCB *currentProcess);
// Global PCB linked list head, used for updating page tables
struct PCB *pcbListHead = NULL;  // When creating a PCB, it needs to be added to this linked list

// Mutex and multithreading flag (declared in interpreter.c)
extern pthread_mutex_t readyQueueMutex;
extern int multithreadEnabled;

// Lock the ready queue
void lockReadyQueue() {
    if (multithreadEnabled) {
        pthread_mutex_lock(&readyQueueMutex);
    }
}

// Unlock the ready queue
void unlockReadyQueue() {
    if (multithreadEnabled) {
        pthread_mutex_unlock(&readyQueueMutex);
    }
}

// Enqueue a process (PCB) to the ready queue
void enqueue(struct PCB *process) {
    lockReadyQueue();
    if (readyQueueHead == NULL) {
        readyQueueHead = process;
        readyQueueTail = process;
    } else {
        readyQueueTail->next = process;
        readyQueueTail = process;
    }
    unlockReadyQueue();
}

// Dequeue a process (PCB) from the ready queue
struct PCB *dequeue() {
    lockReadyQueue();
    if (readyQueueHead == NULL) {
        unlockReadyQueue();
        return NULL;  // Queue is empty
    }
    struct PCB *process = readyQueueHead;
    readyQueueHead = readyQueueHead->next;
    if (readyQueueHead == NULL) {
        readyQueueTail = NULL;  // Queue is now empty
    }
    process->next = NULL;  // Disconnect from the queue
    unlockReadyQueue();
    return process;
}

// Run FCFS Scheduler
void runScheduler() {
    while (readyQueueHead != NULL) {
        struct PCB *currentProcess = dequeue();  // Get the next process

        // Execute the process's instructions
        while (currentProcess->pc < currentProcess->length) {
            char *line = getLineFromPCB(currentProcess);  // Get instruction from PCB's page table

            if (line == NULL) {
                // Page not loaded or has been evicted, handle page fault
                int pageNumber = currentProcess->pc / FRAME_SIZE;
                handlePageFault(currentProcess, pageNumber);

                // Re-fetch the instruction
                line = getLineFromPCB(currentProcess);
                if (line == NULL) {
                    printf("Error: Unable to load instruction for process %d at PC %d.\n",
                           currentProcess->pid, currentProcess->pc);
                    break;
                }
            }

            parseInput(line);  // Execute instruction
            currentProcess->pc++;  // Move to the next instruction
        }

        // Process completed, clean up resources
        destroyPCB(currentProcess);
    }
}

// Run RR Scheduler (time slice of 2)
void runSchedulerRR() {
    while (readyQueueHead != NULL) {
        struct PCB *currentProcess = dequeue();  // Get the next process

        // Execute instructions
        int timeSlice = 0;  // Initialize time slice counter

        while (timeSlice < 2) {  // Time slice is 2
            if (currentProcess->pc >= currentProcess->length) {
                break;  // Process has completed
            }

            int pageNumber = currentProcess->pc / FRAME_SIZE;
            int offset = currentProcess->pc % FRAME_SIZE;

            // Check if the page is loaded
            if (currentProcess->pageTable[pageNumber] == -1) {
                // Handle page fault
                handlePageFault(currentProcess, pageNumber);

                // After handling the page fault, yield control
                // Re-enqueue the current process and move on to the next process
                enqueue(currentProcess);
                break;  // Break out of the time slice loop
            }

            int frameNumber = currentProcess->pageTable[pageNumber];

            // Update frame usage time
            accessFrame(frameNumber);

            // Fetch the instruction
            char *line = getLineFromPCB(currentProcess);
            if (line == NULL) {
                // This should not happen, but handle it gracefully
                printf("Error: Unable to fetch instruction for process %d at PC %d.\n",
                       currentProcess->pid, currentProcess->pc);
                // Re-enqueue the current process and move on to the next process
                enqueue(currentProcess);
                break;  // Break out of the time slice loop
            }

            // Execute the instruction
            parseInput(line);

            // Increment the program counter after successful execution
            currentProcess->pc++;
            timeSlice++;  // Increment the time slice counter
        }

        // Check if the process has completed
        if (currentProcess->pc >= currentProcess->length) {
            destroyPCB(currentProcess);
        } else if (timeSlice == 2) {
            // Process not completed and time slice expired, re-enqueue it
            enqueue(currentProcess);
        }
        // If the process was re-enqueued due to a page fault, it's already in the queue
    }
}

// Enqueue PCB to the ready queue based on SJF strategy
void enqueueSJF(struct PCB *pcb) {
    lockReadyQueue();
    if (readyQueueHead == NULL) {
        readyQueueHead = pcb;
        readyQueueTail = pcb;
    } else {
        struct PCB *prev = NULL;
        struct PCB *current = readyQueueHead;

        // Sort based on script length
        while (current != NULL && current->length <= pcb->length) {
            prev = current;
            current = current->next;
        }

        if (prev == NULL) {
            // Insert at the head
            pcb->next = readyQueueHead;
            readyQueueHead = pcb;
        } else {
            // Insert in the middle or at the tail
            pcb->next = current;
            prev->next = pcb;
            if (current == NULL) {
                readyQueueTail = pcb;  // Update the tail
            }
        }
    }
    unlockReadyQueue();
}

// Run SJF Scheduler
void runSchedulerSJF() {
    while (readyQueueHead != NULL) {
        struct PCB *currentProcess = dequeue();

        // Execute the process's instructions
        while (currentProcess->pc < currentProcess->length) {
            char *line = getLineFromPCB(currentProcess);

            if (line == NULL) {
                // Page not loaded or has been evicted, handle page fault
                int pageNumber = currentProcess->pc / FRAME_SIZE;
                handlePageFault(currentProcess, pageNumber);

                // Re-fetch the instruction
                line = getLineFromPCB(currentProcess);
                if (line == NULL) {
                    printf("Error: Unable to load instruction for process %d at PC %d.\n",
                           currentProcess->pid, currentProcess->pc);
                    break;
                }
            }

            parseInput(line);
            currentProcess->pc++;
        }

        // Process completed, clean up resources
        destroyPCB(currentProcess);
    }
}

// Run SJF with Aging Scheduler
void runSchedulerSJFwithAging() {
    while (readyQueueHead != NULL) {
        struct PCB *currentProcess = dequeue();

        // Execute one instruction (time slice of 1)
        if (currentProcess->pc < currentProcess->length) {
            char *line = getLineFromPCB(currentProcess);

            if (line == NULL) {
                // Page not loaded or has been evicted, handle page fault
                int pageNumber = currentProcess->pc / FRAME_SIZE;
                handlePageFault(currentProcess, pageNumber);

                // Re-fetch the instruction
                line = getLineFromPCB(currentProcess);
                if (line == NULL) {
                    printf("Error: Unable to load instruction for process %d at PC %d.\n",
                           currentProcess->pid, currentProcess->pc);
                    // The process may need to be rescheduled
                    enqueueSJFAging(currentProcess);
                    continue;
                }
            }

            parseInput(line);
            currentProcess->pc++;
        }

        // Aging: decrease jobLengthScore of other processes in the ready queue
        ageReadyQueue(currentProcess);

        // Find the process with the lowest jobLengthScore
        struct PCB *lowestJob = findLowestScoreJob(currentProcess);

        if (currentProcess->pc < currentProcess->length) {
            if (lowestJob != currentProcess) {
                // Not the lowest score, re-enqueue the process
                enqueueSJFAging(currentProcess);
            } else {
                // Lowest score, put it back at the head of the queue
                lockReadyQueue();
                currentProcess->next = readyQueueHead;
                readyQueueHead = currentProcess;
                unlockReadyQueue();
            }
        } else {
            // Process completed, clean up resources
            destroyPCB(currentProcess);
        }
    }
}

// Aging: decrease jobLengthScore of other processes in the ready queue
void ageReadyQueue(struct PCB *currentProcess) {
    lockReadyQueue();
    struct PCB *current = readyQueueHead;
    while (current != NULL) {
        if (current != currentProcess && current->jobLengthScore > 0) {
            current->jobLengthScore--;
        }
        current = current->next;
    }
    unlockReadyQueue();
}

// Find the process with the lowest jobLengthScore
struct PCB* findLowestScoreJob(struct PCB *currentProcess) {
    lockReadyQueue();
    struct PCB *current = readyQueueHead;
    struct PCB *lowestJob = currentProcess;

    while (current != NULL) {
        if (current->jobLengthScore < lowestJob->jobLengthScore) {
            lowestJob = current;
        }
        current = current->next;
    }
    unlockReadyQueue();
    return lowestJob;
}

// Enqueue PCB to the ready queue based on SJF with Aging strategy
void enqueueSJFAging(struct PCB *pcb) {
    lockReadyQueue();
    if (readyQueueHead == NULL) {
        readyQueueHead = pcb;
        readyQueueTail = pcb;
    } else {
        struct PCB *prev = NULL;
        struct PCB *current = readyQueueHead;

        // Sort based on jobLengthScore
        while (current != NULL && current->jobLengthScore <= pcb->jobLengthScore) {
            prev = current;
            current = current->next;
        }

        if (prev == NULL) {
            // Insert at the head
            pcb->next = readyQueueHead;
            readyQueueHead = pcb;
        } else {
            // Insert in the middle or at the tail
            pcb->next = current;
            prev->next = pcb;
            if (current == NULL) {
                readyQueueTail = pcb;  // Update the tail
            }
        }
    }
    unlockReadyQueue();
}

// Run RR30 Scheduler (time slice of 30)
void runSchedulerRR30() {
    while (readyQueueHead != NULL) {
        struct PCB *currentProcess = dequeue();

        // Execute instructions within the time slice
        for (int i = 0; i < 30; i++) {
            if (currentProcess->pc >= currentProcess->length) {
                break;  // Process has ended
            }

            int pageNumber = currentProcess->pc / FRAME_SIZE;
            int offset = currentProcess->pc % FRAME_SIZE;

            // Check if the page is in memory
            if (currentProcess->pageTable[pageNumber] == -1) {
                // Handle page fault
                handlePageFault(currentProcess, pageNumber);

                // Re-fetch frameNumber
                int frameNumber = currentProcess->pageTable[pageNumber];
                if (frameNumber == -1) {
                    // Unable to load the page, skip this process
                    printf("Error: Unable to load page %d for process %d.\n", pageNumber, currentProcess->pid);
                    break;
                }
                accessFrame(frameNumber);
            }

            // Access the frame (update usage time)
            int frameNumber = currentProcess->pageTable[pageNumber];
            accessFrame(frameNumber);

            // Fetch the instruction from PCB
            char *line = getLineFromPCB(currentProcess);
            if (line == NULL) {
                // Unable to fetch instruction, possibly due to page eviction, handle page fault
                handlePageFault(currentProcess, pageNumber);
                // Re-fetch the instruction
                line = getLineFromPCB(currentProcess);
                if (line == NULL) {
                    printf("Error: Unable to load instruction for process %d at PC %d.\n",
                           currentProcess->pid, currentProcess->pc);
                    break;
                }
            }

            parseInput(line);
            currentProcess->pc++;  // Move to the next instruction
        }

        // Check if the process has completed
        if (currentProcess->pc >= currentProcess->length) {
            destroyPCB(currentProcess);
        } else {
            // Process not completed, re-enqueue it
            enqueue(currentProcess);
        }
    }
}

// Enqueue PCB to the head of the ready queue
void enqueueToHead(struct PCB *pcb) {
    lockReadyQueue();
    if (readyQueueHead == NULL) {
        readyQueueHead = pcb;
        readyQueueTail = pcb;
    } else {
        pcb->next = readyQueueHead;
        readyQueueHead = pcb;
    }
    unlockReadyQueue();
}
