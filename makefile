CC = gcc
CFLAGS = -Wall -Wpedantic -std=c99 -D_XOPEN_SOURCE=500 -lrt -lpthread
LDFLAGS = -lm

all: slave vista mainProcess

slave: slave.c
	$(CC) $(CFLAGS) $^ -o $@

vista: vista.c shm_impl.c
	$(CC) $(CFLAGS) $^ -o $@

mainProcess: mainProcess.c shm_impl.c
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

debug: CFLAGS += -g
debug: LDFLAGS += -g
debug: clean all

clean:
	rm -f slave vista mainProcess *.o output.txt
