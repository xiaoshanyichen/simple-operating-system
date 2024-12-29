#ifndef SCHEDULER_H
#define SCHEDULER_H


#include "pcb.h"


// Enqueue a process (PCB) into the ready queue
void enqueue(struct PCB *process);

void enqueueSJF(struct PCB *pcb);

// Dequeue a process (PCB) from the ready queue
struct PCB *dequeue();


// Run the FCFS scheduler to execute processes
void runScheduler();
void runSchedulerRR();
void enqueueSJFAging(struct PCB *pcb);
void runSchedulerSJFwithAging();

void runSchedulerInBackground();
void runSchedulerRRInBackground();
void runSchedulerSJFwithAgingInBackground();
void enqueueToHead(struct PCB *pcb);
void runSchedulerRR30();
void runSchedulerRR30InBackground();




#endif
