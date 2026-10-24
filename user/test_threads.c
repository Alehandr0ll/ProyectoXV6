// user/test_threads.c
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "user/thread.h"

extern struct hebra todas_las_hebras[MAX_THREADS];
extern struct hebra *hebra_actual;

void tarea_hebra(void) {
    int id = hebra_actual - todas_las_hebras;   // main = 0, las demás 1, 2, 3

    //Aquí mostramos que hebra es y en qué iteracipón va, y luego cedemos el turno a otra hebra.
    for (int i = 0; i < 5; i++) {
        printf("Hebra %d: iteracion %d\n", id, i + 1);
        thread_yield();
    }

    // Fin de la hebra: se libera su espacio y se cede el control.
    hebra_actual->estado = LIBRE;
    thread_schedule();

    // si thread_schedule retorna, no quedaba ninguna hebra por seguir ejecutando,
    //  ya terminaron todas, por lo que se cierra el programa y se imprime el mensaje
    printf("Todas las hebras terminaron\n");
    exit(0);
}

int main(int argc, char *argv[]) {
    thread_init();                    // Inicialización

    thread_create(tarea_hebra);       // Creación de 3 hebras
    thread_create(tarea_hebra);
    thread_create(tarea_hebra);

    thread_schedule();                //  Ejecución como tal 
    exit(0);
}