#include "kernel/types.h"//define los tipos de datos básicos y abreviaturas que utiliza el so xv6(enteros sin signo, tamaños etc).
//el resto de librerias dependen de la de types
#include "kernel/stat.h"//da la def del struct stat. esta es la que contiene la planilla para almacenar los metadatos
#include "user/user.h"//contiene las declaraciones de todas las llamadas al sistema.

// Esta funcion termina con exit; no vuelve a quien la llamo.
void filtrar(int entrada) __attribute__((noreturn));

void
filtrar(int entrada)
{
    int primo;

    // Leemos un entero desde la entrada.
    int leidos = read(entrada, &primo, sizeof(primo)); //leidos es la cantidad de bytes leidos

    // Caso base: sin un primer numero, no creamos mas procesos.
    if (leidos == 0) {//el proceso anterior cerro su tuberia porque ya no hay mas numeros que evaluar
        close(entrada);
        exit(0);
    }
    if (leidos != sizeof(primo)) {
        fprintf(2, "Error al leer el primo\n");
        close(entrada);
        exit(1);
    }

    printf("prime %d\n", primo); //marcamos primo encontrado

    // Este pipe conecta ESTE filtro con el siguiente filtro.
    int siguiente[2];//crea una nueva pipe llamada siguiente, 0 para que el futuro hijo lea y 1 para que el proceso actual escriba
    if (pipe(siguiente) < 0) {//filtro para ver si se creo la pipe, pipe(siguiente) siempre returna un entero positivo
        fprintf(2, "Error al crear pipe\n");
        close(entrada);
        exit(1);
    }

    int pid = fork();//vuelve a clonar el proceso actual dividiendo la ejecucion en 2 
    if (pid < 0) { //error al crear hijo
        fprintf(2, "Error al crear hijo\n");
        close(entrada);
        close(siguiente[0]);
        close(siguiente[1]);
        exit(1);
    }

    if (pid == 0) {
        // HIJO: solo lee del nuevo pipe.
        close(siguiente[1]);//cierra su escritura
        // La entrada anterior le corresponde al padre.
        close(entrada);
        // Recursion: el nuevo proceso se encarga del siguiente primo.
        filtrar(siguiente[0]);
    } else {
        // PADRE: lee de entrada y escribe hacia el siguiente filtro.
        close(siguiente[0]);

        int numero;
        while ((leidos = read(entrada, &numero, sizeof(numero))) == sizeof(numero)) {
            //Comprobar si NO es multiplo de primo usando %.
            if (numero % primo != 0){
                //enviamos al hijo, chekeando por si hay error en el envio.
                if (write(siguiente[1], &numero, sizeof(numero)) != sizeof(numero)) {
                    fprintf(2, "No se pudo enviar el numero\n");
                    close(entrada);
                    close(siguiente[1]);
                    wait(0);
                    exit(1);
                }
            }
            //si es divisible, no lo mandamos
        }

        // Solo 0 indica fin de datos.
        if (leidos != 0) {
            fprintf(2, "Error al leer el numero\n");
            close(entrada);
            close(siguiente[1]);
            wait(0);
            exit(1);
        }

        close(entrada);
        // Cerrar ANTES de esperar: el hijo necesita recibir fin de datos.
        close(siguiente[1]);
        wait(0);
        exit(0);
    }
}

int
main(void)
{
    int p[2];
    if (pipe(p) < 0) {//se crea la primera tuberia(pipe) p[0] sera donde se lee y p[1] para escribir
        fprintf(2, "Error al crear pipe inicial\n");
        exit(1);
    }


    //fork llama al sistema operativo que toma el proceso actual (padre) y crea una copia exacta en la memoria (proceso hijo), tienen las mismas variables y mismas tuberias abiertas
    // El pipe existe antes de fork: el hijo hereda sus descriptores.
    int pid = fork();//el programa se clona y la ejecucion se divide en dos caminos paralelos.
    if (pid < 0) {//pid identificador de procesos
        fprintf(2, "Error al crear primer hijo\n");
        close(p[0]);
        close(p[1]);
        exit(1);
    }

    if (pid > 0) {
        // PADRE INICIAL: genera numeros; no filtra ninguno.
        close(p[0]);
        for (int i = 2; i <= 35; i++) {
            // Enviamos los bytes del entero, usando su direccion y tamaño, y revisando por si hay error.
            if (write(p[1], &i, sizeof(i)) != sizeof(i)) {//toma el numero act del ciclo, busca su direccion en memoria &i, y calcula su tamaño en bytes e inyecta los bytes por el extremo de escritura de la tuberiap[1],
                //para que viajen al proceso hijo
                fprintf(2, "No se pudo enviar el numero\n");
                close(p[1]);
                wait(0);
                exit(1);
            }
        }
        close(p[1]);
        wait(0);
        exit(0);
    } else {
        // PRIMER HIJO: inicia la cadena de filtros.
        close(p[1]);
        filtrar(p[0]);
    }
}
