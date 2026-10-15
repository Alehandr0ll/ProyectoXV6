//guardas de inclusión: como las hebras compartiran codigo, si no se usan
//estas guardas, cada hebra llamara una y otra vez a thread.h y el compilador
//dara problemas de compilacion (error de redefinicion).
#ifndef THREAD_H //si no esta definido, se lee sino omite
#define THREAD_H// se crea la etiqueta
//se incluyen los tipos básicos del xv6
#include "kernel/types.h"

#define STACK_SIZE 4096 //tamaño estandar de la pila por cada hebra
#define MAX_THREADS 10 //límite de hebras que soportara el planificador

enum estado_hebra{//requerimiento A
    LIBRE,
    LISTA_PARA_CORRER,
    CORRIENDO
};

//estructura para guardar el contexto en la cpu(requerimiento A)
struct contexto_hebra{
    uint64 ra;//direccion de retorno(cuando la hebra se pause, el procesador guarda ra en la direccion de memoria
    //exacta a la que debe volver cuando termine la llamada)(en que linea del codigo se debe reanudar su ejecucion.)

    uint64 sp;//puntero a la pila, apunta a la direccion de memoria donde esta el tope de su stack(el stack de cada hebra).

    //registros "callee-saved", define como las funciones comparten registros sin sobreescribirse los datos entre si.
    //reserva de espacio para los registros s0 al s11 de la RISC-V, estos son los registros protegidos por la convencion de llamadas, 
    uint64 s0;
    uint64 s1;
    uint64 s2;
    uint64 s3;
    uint64 s4;
    uint64 s5;
    uint64 s6;
    uint64 s7;
    uint64 s8;
    uint64 s9;
    uint64 s10;
    uint64 s11;

    //los caller-saved son registros temporales, si la funcion A llama a la B, la funcion A asume que la funcion B
    //sobreescribira y destruir los registros (completamente distinto a los callee-saved)

    //los callee- saved son protegidos, si la funcion B quiere usar los registros, esta obligada a guardar el valor que
    //tenian antes y dejarlos igual a como estaban antes de retornar a la funcion A. 

    //cuando el codigo decida hacer un cambio de contexto, el compilador de C automaticamente asume que los registros 
    //caller- saved se van a perder asi que el mismo se encarga de proteger lo que se necesite, por lo que guardar los 32 
    //registros(los caller-saved) seria un desperdicio.

};

//estructura principal de la hebra.
struct hebra{
    enum estado_hebra estado; //estado de la hebra
    struct contexto_hebra contexto; //contexto para cuando se pause
    char stack[STACK_SIZE];//pila stack exclusiva de cada hebra 
};

//declaracion de las funciones de las hebras
//inicializa el sistema de hebras
void thread_init(void);
//crea una nueva hebra, asignandole una funcion a ejecutar
void thread_create(void (*func)());
//planificador que decide que hebra ejecutar a continuacion
void thread_schedule(void);
//permite a la hebra ceder su turno de la cpu voluntariamente, la hebra queda en LISTA_PARA_CORRER
void thread_yield(void);

#endif