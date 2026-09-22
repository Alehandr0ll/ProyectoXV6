#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

// Buscamos el archivo 'objetivo' a partir de 'path', en caso de 
// no encontrarlo "inmediatamente" aplicamos recursión para entrar a las 
// carpetas etc.
void find(char *path, char *archivo_buscado){
  char buf[512], *p;
  int fd;
  struct dirent entrada;
  struct stat info_base;      // estado del 'path' que nos pasaron
  struct stat hijo;    // estado de cada entrada que encontremos dentro

  // Abrimos la ruta, 0 es solo lectura.
  if((fd = open(path, 0)) < 0){
    fprintf(2, "find: no se pudo abrir %s\n", path);
    return;
  }

  // Pedimos los datos y se los damos a la variable
  if(fstat(fd, &info_base) < 0){
    fprintf(2, "find: no se pudo hacer fstat de %s\n", path);
    close(fd);
    return;
  }

  switch(info_base.type){

  case T_FILE:
    break;
  case T_DEVICE:
    break;

  case T_DIR:
    // Aquí vemos que la ruta nueva quepa en el buffer
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof(buf)){
      printf("find: ruta demasiado larga\n");
      break;
    }

    // copiamos la ruta que tenemos en el path, y dejamos el puntero apuntando
    //al final de la palabra y agregamos un / al final
    strcpy(buf, path);
    p = buf + strlen(buf);  
    *p++ = '/';              

    // Leemos el directorio, y read debería de entregar el valor de sizeof(de)
    // mientras se ejecute correctamente
    while(read(fd, &entrada, sizeof(entrada)) == sizeof(entrada)){

      //Por si eliminamos un archivo y queda un "espacio vacío"
      //para que continúe la ejecución del ciclo.
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