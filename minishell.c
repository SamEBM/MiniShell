#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <limits.h>
#include <pwd.h>

#define TRUE 1
#define FALSE 0
#define TAM_ARG 5
typedef int bool;

typedef struct {
     char * nombreComando;
     char ** argv;
} comando;

typedef struct {
     int nComandos;
     comando * comandos;
     char * entrada;
     int concatenar;
     char * salida;
} linea;

void ejecutar(char* tokens[]);                       // Funcion que recibe tokens y modifica los atributos de la estructura linea
void parse(char *linea, char **tokens);              // Divide la entrada en tokens (cada palabra es un token)
void imprimeTokens(char* tokens[]);                  // Imprime las palabras  
int numeroComandos(char* tokens[]);                  // Almanacena en nComandos el numero de comandos de la linea ingresada
bool esComando(char* token);                         // Verifica si una cadena es un comando aceptado   
void Salida(char* tokens[], linea* instruccion);     // Verifica si existe redireccionamiento de la salida en la linea ingresada
void Entrada(char* tokens[], linea* instruccion);    // Verifica si existe redireccionamiento de la entrada en la linea ingresada

char* comandos[] = {"exit","clear","cd","ls","wc","cal","cat","cp","date","find","grep","mkdir","mv","ps","pstree",
                    "rev","pwd","sort","uname","wc","which","who","man","df","w","rm","head","echo","whoami"};
int comandosUNIX = sizeof(comandos) / sizeof(comandos[0]);
int N_TOKENS;

void parse(char *linea, char **tokens){
     while (*linea != '\0') {       
          while (*linea == ' ' || *linea == '\t' || *linea == '\n')
               *linea++ = '\0';     
          *tokens++ = linea;         
          while (*linea != '\0' && *linea != ' ' && *linea != '\t' && *linea != '\n') 
               linea++;             
     }
     *tokens = '\0';  //NULL
}

void imprimeTokens(char* tokens[]){
     for (int i = 0; i < 63; i++){
          if(tokens[i] != NULL){
               printf("TOKEN [%i] = [%s]\n",i,tokens[i]);
          }else{
               return;
          }
     }
}

bool esComando(char* token){
     for (int i = 0; i < comandosUNIX; i++){
          if(strcmp(token, comandos[i]) == 0){
               return TRUE;
          }
     }
     return FALSE;     
}  

int numeroComandos(char* tokens[]){
     int numComandos = 0;
     for (int i = 0; i < 63; i++){
          if(tokens[i] == NULL){
               N_TOKENS = i;
               return numComandos;
          }else{
               if(esComando(tokens[i])){
                    numComandos++;
               }               
          }     
     }    
}

void Salida(char* tokens[],linea* instruccion){      // Direccionamiento con ">" o ">>" 
     for (int i = 0; i < 63; i++){
          if(tokens[i] == NULL){
               instruccion->concatenar = 0;
               instruccion->salida = NULL;
               return;
          }else{
               if(strcmp(tokens[i], ">") == 0){
                    instruccion->concatenar = 0;
                    instruccion->salida = tokens[i+1];
                    return;
               }else if(strcmp(tokens[i], ">>") == 0){
                    instruccion->concatenar = 1;
                    instruccion->salida = tokens[i+1];
                    return;
               }               
          }     
     }  
}

void Entrada(char* tokens[],linea* instruccion){      // Direccionamiento con "<"
     for (int i = 0; i < 63; i++){
          if(tokens[i] == NULL){
               instruccion->entrada = NULL;
               return;
          }else{
               if(strcmp(tokens[i], "<") == 0){
                    instruccion->entrada = tokens[i+1];
                    return;
               }             
          }      
     }    
}

void ejecutar(char* tokens[]){
     int i,j=0,k,pid,*pidHijos,status, fichero,**pipes; //Arreglo de hijos, matriz de pipes
     char ** argvAux1 = (char**)malloc(sizeof(char**)), ** argvAux2 = (char**)malloc(sizeof(char**)), ** argvAux3 = (char**)malloc(sizeof(char**)), 
          ** argvAux4 = (char**)malloc(sizeof(char**)), ** argvAux5 = (char**)malloc(sizeof(char**));

     comando arrayComandos[5];
     arrayComandos[0].argv = argvAux1;
     arrayComandos[1].argv = argvAux2;
     arrayComandos[2].argv = argvAux3;
     arrayComandos[3].argv = argvAux4;
     arrayComandos[4].argv = argvAux5;

     linea* instruccion = (linea*)malloc(sizeof(linea));
     instruccion->nComandos = numeroComandos(tokens);
     instruccion->comandos = arrayComandos; //Asignamos el arreglo de comandos

     if(instruccion->nComandos == 0){
          printf("ERROR: No se ingreso ningun comando valido\n\n");
          exit(-1);
     }

     for(i=0; i< instruccion->nComandos; i++){ //Utilizamos solo los necesarios
          instruccion->comandos[i].argv = arrayComandos[i].argv;
     }

     Entrada(tokens,instruccion);
     Salida(tokens,instruccion);

     //printf("\n# Comandos: [%d]\n",instruccion->nComandos);
     //printf("\n# N_TOKENS: [%d]\n",N_TOKENS);

     for(i=0;i < instruccion->nComandos; i++){    
          if(j < N_TOKENS){
               if(esComando(tokens[j])){
                    instruccion->comandos[i].nombreComando = tokens[j];
                    //printf("Comando: [%i]  Nombre: [%s]\n",i,instruccion->comandos[i].nombreComando);
                    k=0;     
                    while(strcmp(tokens[j],"|") != 0 && strcmp(tokens[j],">") != 0 && 
                         strcmp(tokens[j],">>") != 0 && strcmp(tokens[j],"<") != 0){
     
                         instruccion->comandos[i].argv[k] = tokens[j];
                         //printf("Comando [%i] Argumento [%i]: [%s]\n",i,k,instruccion->comandos[i].argv[k]);
                         //printf("I[%i] J[%i] K[%i]\n",i,j,k);  
                         k++; j++;

                         if(j >= N_TOKENS){
                              break;
                         }                  
                    }
                    if(strcmp(instruccion->comandos[0].nombreComando,"which")==0 || strcmp(instruccion->comandos[0].nombreComando,"man")==0){
                         instruccion->nComandos = 1;
                    }
                    instruccion->comandos[i].argv[k] = NULL;
               }else{
                    i--; j++;
               }
          }
     }

     i=0;

     if (strcmp(instruccion->comandos[0].argv[0], "exit") == 0){
               exit(0);       
          }else if(strcmp(instruccion->comandos[0].argv[0], "cd") == 0){
               int error = chdir(instruccion->comandos[0].argv[1]); // Nos lleva al destino solicitado por el primer argumento

               if (error == -1){ // Si no existe ese destino
                    printf("ERROR: El directorio no existe: %s \n", instruccion->comandos[0].argv[1]);
               }

          /*----------------------------------1 COMANDO----------------------------------*/

          }else if(instruccion->nComandos == 1){ //Si sólo hay un comando
               
               pid = fork();

               if (pid < 0){     // Crea un proceso hijo
                    perror("Error en Fork");
                    exit(-1);
               }else if (pid == 0){
                    if(instruccion->entrada != NULL){ //si hay redireccion de entrada
                              int fichero;
                              fichero = open(instruccion->entrada, O_RDONLY);
                              if (fichero == -1) {
                                   fprintf(stderr,"%i: Error. Fallo al abrir el fichero de redirección de entrada\n", fichero);
                                   exit(1);
                              } else {
                                   dup2(fichero,0); //redirijimos la entrada
                              }
                         if (instruccion->salida != NULL) { //si hay redireccion de salida
                              int fichero2;
                              
                              if(instruccion->concatenar == 1){ //si hay un >>
                                   fichero2 = open(instruccion->salida, O_WRONLY | O_CREAT | O_APPEND, 0600);
                              }else{
                                   fichero2 = open(instruccion->salida, O_WRONLY | O_CREAT | O_TRUNC , 0600);
                              }
                              
                              if (fichero2 == -1) {
                                   fprintf(stderr,"%i: Error. Fallo al abrir el fichero de redirección de salida\n", fichero2);
                                   exit(1);
                              } else {
                                   dup2(fichero2,1); //redirigimos la salida
                              }
                              execvp(instruccion->comandos[0].nombreComando, instruccion->comandos[0].argv); //ejecutamos el comando
                              fprintf(stderr,"%s: No se encuentra el comando\n",instruccion->comandos[i].nombreComando);
                         }else { //si solo hay redireccion de entrada
                              execvp(instruccion->comandos[0].nombreComando, instruccion->comandos[0].argv); //ejecutamos el comando
                              fprintf(stderr,"%s: No se encuentra el comando\n",instruccion->comandos[i].nombreComando);
                         }
                    } else if (instruccion->salida != NULL) { //si solo hay redireccion de salida
                         int fichero;
                         
                         if(instruccion->concatenar == 1){ //si hay un >>
                              fichero = open(instruccion->salida, O_WRONLY | O_CREAT | O_APPEND, 0600);
                         }else{
                              fichero = open(instruccion->salida, O_WRONLY | O_CREAT | O_TRUNC , 0600);
                         }

                         if (fichero == -1) {
                              fprintf(stderr,"%i: Error. Fallo al abrir el fichero de redirección de salida\n", fichero);
                              exit(1);
                         } else {
                              dup2(fichero,1); //redirigimos la salida
                         }
                         execvp(instruccion->comandos[0].nombreComando, instruccion->comandos[0].argv); //ejecutamos el comando
                         fprintf(stderr,"%s: No se encuentra el comando\n",instruccion->comandos[i].nombreComando);
                    } else { //si no hay redirecciones
           
                         execvp(instruccion->comandos[0].nombreComando, instruccion->comandos[0].argv); //ejecutamos el comando
                         perror("\nError en exec\n");
                         fprintf(stderr,"%s: No se encuentra el comando\n",instruccion->comandos[i].nombreComando);
                         exit(-1);
                    }

               }else{ //el padre
                    while (wait(&status) != pid)
                    ; //espera por su hijo
               }

          /*-----------------------------------------------------------------------------*/

          /*-------------------------------2 O MAS COMANDOS------------------------------*/     
          }else if(instruccion->nComandos >= 2){ 
               pidHijos = malloc(instruccion->nComandos * sizeof(int)); 
               pipes = (int **) malloc ((instruccion->nComandos-1) * sizeof(int *)); //Reservamos memoria para la matriz de pipes

               for(i=0; i<instruccion->nComandos-1; i++){ //Creo tantos pipes como comandos-1
                         pipes[i] = (int *) malloc (2*sizeof(int)); // MATRIZ: nComandos-1 * 2 
                         if(pipe(pipes[i]) < 0)
                              perror("Error en Pipe");
               }
               for(i=0; i < instruccion->nComandos; i++){ //Por cada comando se crea un proceso HIJO   
                    pid = fork();

                    if(pid < 0){
                         perror("Error en Fork");
                         exit(-1);
                    } else if(pid == 0){ //Soy el HIJO
                         if(i == 0){ //Si soy el primer comando
                              if(instruccion->entrada != NULL){ //Si hay redireccion de entrada
                                   fichero = open(instruccion->entrada, O_RDONLY);
                                   if (fichero == -1) {
                                        fprintf(stderr,"%i: Error. Fallo al abrir el fichero de redirección de entrada\n", fichero);
                                        exit(1);
                                   } else {
                                        dup2(fichero,0); //Redirijimos la entrada
                                   }
                              }
                              for(j=1; j<instruccion->nComandos-1; j++){ //Por cada pipe creado (menos el que voy a utilizar) cierro su p[1] y p[0] 
                                   close(pipes[j][1]);
                                   close(pipes[j][0]);
                              }
                              close(pipes[0][0]);
                              dup2(pipes[0][1],1); //Escritura del pipe como "salida estandar", la salida de la ejecucion se pasa al pipe
                         }
                         if(i>0 && i<instruccion->nComandos-1){ //Si soy un comando intermedio   
                              if(i==1 && instruccion->nComandos != 3){ //Si soy el segundo comando
                                   for(j=i+1; j<instruccion->nComandos-1; j++){ //Por cada pipe creado (menos el que voy a utilizar) cierro su p[1] y p[0] 
                                        close(pipes[j][1]);
                                        close(pipes[j][0]);
                                   }                             
                              }
                              if(i==instruccion->nComandos-2 && instruccion->nComandos != 3){ //Si soy el penultimo comando
                                   for(j=0; j<i-1; j++){ //Por cada pipe creado (menos el que voy a utilizar) cierro su p[1] y p[0] 
                                        close(pipes[j][1]);
                                        close(pipes[j][0]); 
                                   }
                              }
                              if(i!=1 && i!=instruccion->nComandos-2 && instruccion->nComandos != 3){ //Si no soy ni el segundo ni el penultimo
                                   for(j=0; j<i-1; j++){ //Por cada pipe creado (menos el que voy a utilizar) cierro su p[1] y p[0] 
                                        close(pipes[j][1]);
                                        close(pipes[j][0]);
                                   }
                                   for(j=i+1; j<instruccion->nComandos-1; j++){ //Por cada pipe creado (menos el que voy a utilizar) cierro su p[1] y p[0] 
                                        close(pipes[j][1]);
                                        close(pipes[j][0]);
                                   }
                              }
                              close(pipes[i-1][1]);    //Cierra la escritura del pipe anterior
                              dup2(pipes[i-1][0],0);   //Reemplaza la entrada estandar por la lectura del pipe anterior
                              close(pipes[i][0]);      //Cierra la lectura del pipe actual
                              dup2(pipes[i][1],1);     //Reemplaza la salida estandar por la escritura del pipe actual
                         }
                         if(i == instruccion->nComandos-1){ //Si soy el ultimo comando
                              if (instruccion->salida != NULL) { //Si hay redireccion de salida
                                   
                                   if(instruccion->concatenar == 1){ //si hay un >>
                                        fichero = open(instruccion->salida, O_WRONLY | O_CREAT | O_APPEND, 0600);
                                   }else{
                                        fichero = open(instruccion->salida, O_WRONLY | O_CREAT | O_TRUNC , 0600);
                                   }
                                   if (fichero == -1) {
                                        fprintf(stderr,"%i: Error. Fallo al abrir el fichero de redirección de salida\n", fichero);
                                        exit(1);
                                   } else {
                                        dup2(fichero,1); //Redirigimos la salida
                                   }
                              }
                              
                              for(j=0; j<instruccion->nComandos-2; j++){ //Por cada pipe creado (menos el que voy a utilizar) cierro su p[1] y p[0]
                                   close(pipes[j][1]);
                                   close(pipes[j][0]);
                              }
                              close(pipes[i-1][1]);   //Cierra la escritura del pipe anterior
                              dup2(pipes[i-1][0],0);  //Reemplaza la entrada estandar por la lectura del pipe anterior
                         }
                         execvp(instruccion->comandos[i].nombreComando, instruccion->comandos[i].argv); //Ejecutamos el comando
                         fprintf(stderr,"%s: No se encontro el comando\n",instruccion->comandos[i].nombreComando);
                    } else { //Soy el PADRE
                         pidHijos[i] = pid; //Guardamos el PID del HIJO
                    }
               }
               for(k=0; k<instruccion->nComandos-1; k++){ //Cerramos todos los pipes
                    close(pipes[k][1]);
                    close(pipes[k][0]);
               }
               for(k=0; k<instruccion->nComandos; k++){ 
                    waitpid(pidHijos[k],NULL,0); //Espera por todos sus hijos (pid,status,options)
               }
               for(i=0; i<instruccion->nComandos-1; i++){ //Liberamos memoria
                    free(pipes[i]);
               }
               free(pipes);
               free(pidHijos);
          }
          
          /*-----------------------------------------------------------------------------*/

     return;
}

void main(void){
     char line[1024];             // LINEA COMPLETA
     char *argv[64];              // TOKENS
     char usuario[20], computadora[20], directorio[1024];
     
     register struct passwd *pw;
     register uid_t uid;
     uid = geteuid();
     pw = getpwuid(uid);

     gethostname(computadora, sizeof(computadora));

     while (1) {
          getcwd(directorio, sizeof(directorio));                   
          printf("\n%s @ %s : %s $ ",pw->pw_name,computadora,directorio);     
          gets(line);              // Lee el input
          printf("\n");
          parse(line, argv);       // Divide la cadena en Tokens
          //imprimeTokens(argv);
          
          ejecutar(argv);
     }
}
