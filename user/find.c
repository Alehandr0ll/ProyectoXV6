#include "kernel/types.h"//define los tipos de datos básicos y abreviaturas que utiliza el so xv6(enteros sin signo, tamaños etc).
//el resto de librerias dependen de la de types
#include "kernel/stat.h"//da la def del struct stat. esta es la que contiene la planilla para almacenar los metadatos de los archivos(inodo, tamaño, tipo)
//es la que nos da el tipo de de archivo (T_DIR: directorios, T_FILE: archivos normales, T_DEVICE: para dispositivos)
#include "user/user.h"//contiene las declaraciones de todas las llamadas al sistema (open, read, write, close, pipe, fork y wait)
#include "kernel/fs.h"//esta sirve para interactuar con el sistema de archivos

// Buscamos el archivo 'objetivo' a partir de 'path', en caso de 
// no encontrarlo "inmediatamente" aplicamos recursión para entrar a las 
// carpetas etc.
void find(char *path, char *archivo_buscado){// recibe la ruta donde empezar a buscar y el nombre del archivo
  char buf[512], *p;//el buff es para ir construyendo las rutas de los archivos, el puntero p es para despues manipular cieras partes del buff
  int fd;//descriptr del archivo(file descriptor), numero que identificara al directorio cuando se abra
  struct dirent entrada;// struct que tiene la forma de una linea de directorio (inum, name)
  struct stat info_base;      // estado del 'path' que nos pasaron, va a contener los metadatos de un archivo
  struct stat hijo;    // estado de cada entrada que encontremos dentro

  // Abrimos la ruta, 0 es solo lectura.
  if((fd = open(path, 0)) < 0){//open(path,0) abre la ruta en modo lectura, esto genera un ticket de acceso (num entero positivo), si falla genera un negativo
    fprintf(2, "find: no se pudo abrir %s\n", path);
    return;//verifica si se pudo abrir
  }

  // Pedimos los datos y se los damos a la variable
  if(fstat(fd, &info_base) < 0){//pide extraer la informacion del archivo fd y guardarla en info_base
    fprintf(2, "find: no se pudo hacer fstat de %s\n", path);
    close(fd);//si da error manda numero negativo
    return;
  }

  switch(info_base.type){//saca de la info del archivo que recien abrimos el tipo

  case T_FILE:
    break;
  case T_DEVICE:
    break;

  case T_DIR:
    // Aquí vemos que la ruta nueva quepa en el buffer
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof(buf)){//strlen calcula cuantas letras tiene la ruta base en la que estamos parados, suma un espacio para la barra diagonal,
      //DIRSIZ se suma que es la cantidad max de caracteres que puede tener un nombre de archivo en el so, se vuelve a sumar 1 por el caracter nulo /0
      printf("find: ruta demasiado larga\n");
      break;
    }

    // copiamos la ruta que tenemos en el path, y dejamos el puntero apuntando
    //al primer espacio vacio listo para empezar a escribir y agregamos un / al final
    strcpy(buf, path);
    p = buf + strlen(buf);  
    *p++ = '/';              

    // Leemos el directorio, y read debería de entregar el valor de sizeof(de)
    // mientras se ejecute correctamente
    while(read(fd, &entrada, sizeof(entrada)) == sizeof(entrada)){

      //Por si eliminamos un archivo y queda un "espacio vacío"
      //para que continúe la ejecución del ciclo. el inum queda como 0 y los skipea
      if(entrada.inum == 0)       
        continue;

      //Pegamos el nombre justo despues del '/' y lo terminamos en '\0'.
      //Aquí si usaba strcpy arriesgaba posibles errores al depender de encontrar el '\0'
      memmove(p, entrada.name, DIRSIZ);
      p[DIRSIZ] = 0;
      // Ahora buf contiene la ruta completa, y p apunta solo al nombre.

      // comparamos los valores para evitar los bucles infinitos que
      //  nos puede generar '.' o '..' y por eso dejamos el continue
      if(strcmp(p, ".") == 0 || strcmp(p, "..") == 0)
        continue;

      // Lo mismo que arriba pero esta vez con lo que "sí" queremos
      if(strcmp(p, archivo_buscado) == 0)
        printf("%s\n", buf);

      // Nos piden todas las "coincidencias" así que revisamos si también es una carpeta. Y basicamente con stat sacamos los datos necesarios
      if(stat(buf, &hijo) < 0){
        fprintf(2, "find: error al intentar hacer stat %s\n", buf);
        continue;
      }
      // Si resulta que también es una carpeta realizamos la recursión.
      if(hijo.type == T_DIR)
        find(buf, archivo_buscado);
    }
    break;
  }

  close(fd);
}

int main(int argc, char *argv[]){
  if(argc != 3){
    fprintf(2, "el formato debe ser: find <directorio> <nombre_archivo>\n");
    exit(1);
  }
  find(argv[1], argv[2]);
  exit(0);
}