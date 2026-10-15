#include "kernel/types.h"
#include "user/user.h"
#include "thread.h"

struct hebra todas_las_hebras[MAX_THREADS];
struct hebra *hebra_actual; //puntero a la hebra que se esta ejecutando en el momento


extern void swtch(struct contexto_hebra *antigua, struct contexto_hebra *nueva);

void thread_init(void){
    for(int i = 0; i< MAX_THREADS; i++){
        todas_las_hebras[i].estado = LIBRE;
    }

    //la hebra principal(la primera del main esta corriendo, asi que se le asigna el primer espacio
    //y la ponemos corriendo)

    hebra_actual = &todas_las_hebras[0];
    hebra_actual->estado = CORRIENDO;
}

void thread_create(void (*func)()){
    struct hebra *t = 0;
    
    for(int i = 0; i < MAX_THREADS; i++){//se busca un espacio libre en el arreglo de las hebras para crearla
        if(todas_las_hebras[i].estado == LIBRE){
            t = &todas_las_hebras[i];
            break;
        }
    }
    if(t==0){
        return;//noo hay espacio libre
    }

    t->estado = LISTA_PARA_CORRER;
    t->contexto.ra = (uint64) func;
    t->contexto.sp = (uint64) t->stack + STACK_SIZE;

}

void thread_yield(void){
    hebra_actual->estado = LISTA_PARA_CORRER; //se cambia el estado de la hebra actual a lista para ceder la cpu

    thread_schedule(); //llamada al planificador para que la otra hebra tome el control.
}

void thread_schedule(void){
    struct hebra *t;
    struct hebra *hebra_antigua = hebra_actual;

    //se busca en el arreglo de las hebras si hay alguna lista para correr
    for(int i = 0; i < MAX_THREADS; i++){
        int index = (hebra_antigua - todas_las_hebras + i + 1) % MAX_THREADS;//es memoria contigua, se busca desde la hebra que quedo en listo para adelante.
        t = &todas_las_hebras[index];

        if(t->estado == LISTA_PARA_CORRER){
            t->estado = CORRIENDO;
            hebra_actual = t;

            //llamamos a la funcion del ensamblador RISC-V, guardara el contexto de la hebra antigua y 
            //cargara los de la hebra actual.
            swtch(&hebra_antigua->contexto, &hebra_actual->contexto);
            return;
        }

    }
}

