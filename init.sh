# El flag -lm es para usar math.h y que calcule la cantidad de esclavos a utilizar

gcc -Wall -Wpedantic -std=c99  slave.v -o slave
gcc -Wall -Wpedantic -std=c99 -lm mainProcess.v -o mainProcess

# Comando para debuggear con valgrind
# valgrind --trace-children=yes -v --track-origins=yes --leak-check=yes --xml-file="errors.xml" ./mainProcess <argumentos>

# Para debuggear con valgrind, hay que compilar cone el flag -g y usar el codigo objeto como referencia para el programa

# gcc -Wall -Wpedantic -std=c99 -c -g slave.c
# gcc -Wall -Wpedantic -std=c99 -c -g -lm mainProcess.c
# gcc -Wall -Wpedantic -std=c99  slave.o -o slave
# gcc -Wall -Wpedantic -std=c99 -lm mainProcess.o -o mainProcess