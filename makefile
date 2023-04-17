CC = gcc
CFLAGS = -Wall -Wpedantic -std=c99 -D_XOPEN_SOURCE=500 -lrt -lpthread
LDFLAGS = -lm

all: comp slave vista mainProcess cleano

comp: slave.c vista.c shm_impl.c mainProcess.c
	$(CC) $(CFLAGS) -c $^

slave: slave.o
	$(CC) $(CFLAGS) $^ -o $@

vista: vista.o shm_impl.o
	$(CC) $(CFLAGS) $^ -o $@

mainProcess: mainProcess.o shm_impl.o
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

debug: CFLAGS += -g
debug: LDFLAGS += -g
debug: comp slave vista mainProcess

cleano:
	rm -f *.o

clean:
	rm -f slave vista mainProcess *.o output.txt strace_out PVS-Studio.log report.tasks errors.xml
