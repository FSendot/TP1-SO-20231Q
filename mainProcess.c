#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>

#define STDIN 0
#define STDOUT 1
#define ERROR -1

#define MAX_PER_SLAVE 2

int main(int argsc, char argsv[]) {
    //argsv[] = {"./archivo1", "./archivo1"} 

    int slavesCount = (int) (argsc / MAX_PER_SLAVE);


    for (int i = 0; i < slavesCount; i++) {




    }





    return 0;
}