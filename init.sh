gcc -Wall -std=c99  slave.c -o slave
gcc -Wall -std=c99  mainProcess.c -o mainProcess.o
./mainProcess.o ./mainProcess.c
