# TP1-SO-20231Q

## Compilación y Ejecuciónn

Para compilar y linkeditar el programa sólo para ejecución, utilizar el comando `make all`.
Una vez completado, ejecutar el programa mediante el comando `./mainProcess <argumentos>`, donde los argumentos son sólo archivos (el programa no puede procesar directorios), fallará si se intenta realizar esto mismo.

Mientras se ejecuta, el proceso dejará un número impreso en pantalla, el mismo servirá para poder ejecutar mientras el programa corre el comando `./vista <número>` en otra terminal o mientras el programa principal se ejecuta en background.

Por último, se puede ejecutar el programa pipeando la salida del proceso principal a la entrada estándar del proceso vista, de la forma `./mainProcess <argumentos> | ./vista`.

## Compilación para Debugging

Para compilar con fines de debugging, utilizar el comando `make debug`. Este conservará todos los archivos `*.o` y se compilará y linkeditará con el flag `-g` para poder ser usados con los programas de debugging que corresponda.

### Aclaraciones finales
Una vez esté todo completado, puede ejecutar el comando `make clean`, que eliminará todos los archivos presentes en el directorio, a excepción de los códigos fuentes del programa (incluye el borrado de output.txt).
