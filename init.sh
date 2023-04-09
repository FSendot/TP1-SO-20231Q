gcc -Wall -Wpedantic -std=c99  slave.c -o slave
gcc -Wall -Wpedantic -std=c99 -lm mainProcess.c -o mainProcess