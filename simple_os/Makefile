CC = gcc
CFLAGS =

framesize ?= 300
varmemsize ?= 10

CFLAGS += -D FRAME_STORE_SIZE=$(framesize) -D VARIABLE_STORE_SIZE=$(varmemsize)

mysh: shell.c interpreter.c shellmemory.c pcb.c scheduler.c
	$(CC) $(CFLAGS) -c shell.c interpreter.c shellmemory.c pcb.c scheduler.c
	$(CC) $(CFLAGS) -o mysh shell.o interpreter.o shellmemory.o pcb.o scheduler.o

clean:
	rm -f mysh; rm -f *.o
