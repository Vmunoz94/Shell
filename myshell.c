/****************************************************************
 * Name        : Victor Muñoz                                   *
 * Class       : CSC 415                                        *
 * Date  	     : 3/1/2018                                       *
 * Description : Writing a simple bash shell program            *
 * 	        	  that will execute simple commands. The main    *
 *               goal of the assignment is working with         *
 *               fork, pipes and exec system calls.             *
 ****************************************************************/

#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>

#define BUFFERSIZE 256
#define PROMPT "myShell >> "
#define PROMPTSIZE sizeof(PROMPT)
#define ARGVMAX 64
#define PIPECNTMAX 10



int main(int argc, char** argv)
{
     int myargc = 1;
     char** myargv = malloc(ARGVMAX);
     //to use for cd
     char* path;
     //to use for pwd
     char cwd [100];
     //to use for pipe
     int leftArgc = 0;
     char** leftArgv = malloc(ARGVMAX);
     int rightArgc = 0;
     char** rightArgv = malloc(ARGVMAX);
     //to use for waitpid
     int status;
     //to set buffer size
     char buffer [BUFFERSIZE];
     //to store token
     char* token;
     //to store child process id
     pid_t pid, pidTwo;
     //to connect two process's with a pipe
     //fd[0] will be the fd(file descriptor) for the read end of pipe.
     //fd[1] will be the fd(file descriptor) for the write end of pipe.
     int fd[2];

     bool singleRightBracket = false;
     bool singleRightBracketPipeOne = false;
     bool singleRightBracketPipeTwo = false;
     bool doubleRightBracket = false;
     bool doubleRightBracketPipeOne = false;
     bool doubleRightBracketPipeTwo = false;
     bool singleLeftBracket = false;
     bool singleLeftBracketPipeOne = false;
     bool singleLeftBracketPipeTwo= false;
     bool hasPipe = false;
     char* fileName;
     char* fileNameTwo;
     int fileDescriptor = -1;
     int fileDescriptorTwo = -1;
     bool ampersand = false;
     bool loop = true;
     int index = 0;
     int leftIndex = 0;
     int rightIndex = 0;

     while(loop){

          printf(PROMPT);

          /* Exit when user enters control D or exit */
          /* Obtain user input and store in buffer */
          //If an error occurs, a null pointer is returned.
          //(i.e. the user enters CTL-­­D or enters "exit")
          if(gets(buffer) == NULL){
               printf("\n");
               break;
          }
          if(strcmp(buffer, "exit") == 0){
               break;
          }

          /* Pass buffer into command line arguments */
          //parse the user’s line of input into a series of strings that are stored into a char argv** array,
          //along with a count of the number of strings in an int argc variable.
          myargv[index] = strtok(buffer, " ");
          while(myargv[index] != NULL){
               index++;
               myargv[index] = strtok(NULL, " ");
               myargc++;
          }

          /* If first token contains cd */
          //change directory and reset
          if(myargc > 2 && strcmp(myargv[0],"cd") == 0){
               path = myargv[1];
               if(chdir(path) == -1){
                    perror("Not a directory");
               }
               /* Reset */
               index = 0;
               myargc = 1;
               while(strcmp(buffer, "\n") == 0);
               continue;
          }

          /* If first token contains pwd */
          //print working directory
          if(myargc > 1 && strcmp(myargv[0],"pwd") == 0){
               if (getcwd(cwd, sizeof(cwd)) != NULL)
                    printf("Current working directory: %s\n", cwd);
               else
                    perror("getcwd failed: ");
               /* Reset */
               index = 0;
               myargc = 1;
               while(strcmp(buffer, "\n") == 0);
               continue;
          }

          /* If the token contains | */
          // copy argv contents into leftArgv up until | token
          // copy argv contents into rightArgv after | token
          // then set the last token to null
          for(int i = 0; i < myargc - 1; i++){
               if(myargc > 1 && (strcmp(myargv[i], "|") == 0)){
                    hasPipe = true;
                    for(int j = 0; j < i; j++){
                         leftArgv[j] = myargv[j];
                         leftArgv[j+1] = NULL;
                         leftArgc++;
                    }
                    for(int j = leftArgc + 1; j < myargc - 1; j++){
                         rightArgv[rightIndex] = myargv[j];
                         rightIndex++;
                         rightArgc++;
                    }
               }
          }
          leftArgv[leftArgc] = NULL;
          leftArgc++;
          rightArgv[rightArgc] = NULL;
          rightArgc++;

          /* If the last token is &, Execute command in background */
          if(myargc > 1  && (strcmp(myargv[myargc - 2], "&") == 0)){
               ampersand = true;
               myargv[myargc - 2] = NULL;
               myargc--;
          }
          if(rightArgc > 1  && (strcmp(rightArgv[rightArgc - 2], "&") == 0)){
               ampersand = true;
               rightArgv[rightArgc - 2] = NULL;
               rightArgv--;
          }

          /* If the token contains > */
          if(myargc > 2  && (strcmp(myargv[myargc - 3], ">") == 0)){
               singleRightBracket = true;
               myargv[myargc - 3] = NULL;
               fileName = myargv[myargc - 2];
               myargc--;
               myargv[myargc - 1] = NULL;
               myargc--;
          }
          if(leftArgc > 2 && (strcmp(leftArgv[leftArgc - 3], ">") == 0)){
               singleRightBracketPipeOne = true;
               leftArgv[leftArgc - 3] = NULL;
               fileName = leftArgv[leftArgc - 2];
               leftArgc--;
               leftArgv[leftArgc - 1] = NULL;
               leftArgc--;
          }
          if(rightArgc > 2 && (strcmp(rightArgv[rightArgc - 3], ">") == 0)){
               singleRightBracketPipeTwo = true;
               rightArgv[rightArgc - 3] = NULL;
               fileNameTwo = rightArgv[rightArgc - 2];
               rightArgc--;
               rightArgv[rightArgc - 1] = NULL;
               rightArgc--;
          }

          /* If the token contains >> */
          if(myargc > 2  && (strcmp(myargv[myargc - 3], ">>") == 0)){
               doubleRightBracket = true;
               myargv[myargc - 3] = NULL;
               fileName = myargv[myargc - 2];
               myargc--;
               myargv[myargc - 1] = NULL;
               myargc--;
          }
          if(leftArgc > 2 && (strcmp(leftArgv[leftArgc - 3], ">>") == 0)){
               doubleRightBracketPipeOne = true;
               leftArgv[leftArgc - 3] = NULL;
               fileName = leftArgv[leftArgc - 2];
               leftArgc--;
               leftArgv[leftArgc - 1] = NULL;
               leftArgc--;
          }
          if(rightArgc > 2 && (strcmp(rightArgv[rightArgc - 3], ">>") == 0)){
               doubleRightBracketPipeTwo = true;
               rightArgv[rightArgc - 3] = NULL;
               fileNameTwo = rightArgv[rightArgc - 2];
               rightArgc--;
               rightArgv[rightArgc - 1] = NULL;
               rightArgc--;
          }

          /* If the token contains < */
          if(myargc > 2  && (strcmp(myargv[myargc - 3], "<") == 0)){
               singleLeftBracket = true;
               myargv[myargc - 3] = NULL;
               fileName = myargv[myargc - 2];
               myargc--;
               myargv[myargc - 1] = NULL;
               myargc--;
          }
          if(leftArgc > 2 && (strcmp(leftArgv[leftArgc - 3], "<") == 0)){
               singleLeftBracketPipeOne = true;
               leftArgv[leftArgc - 3] = NULL;
               fileName = leftArgv[leftArgc - 2];
               leftArgc--;
               leftArgv[leftArgc - 1] = NULL;
               leftArgc--;
          }
          if(rightArgc > 2 && (strcmp(rightArgv[rightArgc - 3], "<") == 0)){
               singleLeftBracketPipeTwo = true;
               rightArgv[rightArgc - 3] = NULL;
               fileNameTwo = rightArgv[rightArgc - 2];
               rightArgc--;
               rightArgv[rightArgc - 1] = NULL;
               rightArgc--;
          }

          // //Print contents of argv and argc
          // printf("Contents of myargv: ");
          // for(int i = 0; i < myargc; i++){
          //      if(i+1 == myargc){
          //           printf("%s\n", myargv[i]);
          //      }
          //      else{
          //           printf("%s ", myargv[i]);
          //
          //      }
          // }
          // printf("Count of myargc: %d\n", myargc);
          //
          // //Print contents of leftArgv and leftArgc
          // printf("Contents of leftArgv: ");
          // for(int i = 0; i < leftArgc; i++){
          //      if(i+1 == leftArgc){
          //           printf("%s\n", leftArgv[i]);
          //      }
          //      else{
          //           printf("%s ", leftArgv[i]);
          //
          //      }
          // }
          // printf("Count of leftArgc: %d\n", leftArgc);
          //
          // //Print contents of rightArgv and rightArgc
          // printf("Contents of rightArgv: ");
          // for(int i = 0; i < rightArgc; i++){
          //      if(i+1 == rightArgc){
          //           printf("%s\n", rightArgv[i]);
          //      }
          //      else{
          //           printf("%s ", rightArgv[i]);
          //
          //      }
          // }
          // printf("Count of rightArgc: %d\n", rightArgc);

          //If fork() returns a negative value, the creation of a child process was unsuccessful.
          //fork() returns a zero to the newly created child process.
          if((pid = fork())< 0){
               perror("Failed to fork: ");
               return 1;
          }

          /* Child Process */
          if(pid == 0){

               // If token has |
               if(hasPipe){

                    //pipe() returns : 0 on Success. -1 on error.
                    if(pipe(fd) < 0){
                         perror("Failed to create pipe: ");
                         return 1;
                    }
                    //Fork a second child process
                    if((pidTwo = fork())< 0){
                         perror("Failed to fork: ");
                         return 1;
                    }

                    /* Second Child Process */
                    if (pidTwo == 0){
                         //fd[0]- The read end of the pipe is the source file descriptor.
                         //This remains open after the call to dup2
                         //STDIN_FILENO- The destination file descriptor.
                         //This file descriptor will point to the same file as fd[0] after this call returns.
                         if(dup2(fd[0], STDIN_FILENO) < 0){
                              perror("ERROR dup2 failed\n");
                         }
                         //close other end of the pipe
                         close(fd[1]);

                         /* Output to a file when argv contains > */
                         //If file exists it'll be replaced.
                         if(singleRightBracketPipeTwo){
                              fileDescriptorTwo = open(fileNameTwo, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
                              if (fileDescriptorTwo == -1) {
                                   perror("ERROR opening the file:");
                                   return -1;
                              }
                              //fileDescriptorTwo remains open after dup2 is called
                              //STDOUT_FILENO now points to fileDescriptorTwo
                              if(dup2(fileDescriptorTwo, STDOUT_FILENO) < 0){
                                   perror("ERROR dup2 failed\n");
                              }
                              /* COMMAND LINE ARGUMENTS */
                              // Second child process terminates after completion
                              if (execvp(*rightArgv, rightArgv) < 0) {
                                   printf("*** ERROR: exec failed\n");
                                   exit(1);
                              }
                         }
                         /* Output to a file when argv contains >> */
                         //If file exists it'll be appended.
                         if(doubleRightBracketPipeTwo){
                              fileDescriptorTwo = open(fileNameTwo, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
                              if (fileDescriptorTwo == -1) {
                                   perror("ERROR opening the file:");
                                   return -1;
                              }
                              //fileDescriptorTwo remains open after dup2 is called
                              //STDOUT_FILENO now points to fileDescriptorTwo
                              if(dup2(fileDescriptorTwo, STDOUT_FILENO) < 0){
                                   perror("ERROR dup2 failed\n");
                              }
                              /* COMMAND LINE ARGUMENTS */
                              //Child process terminates after completion
                              if (execvp(*rightArgv, rightArgv) < 0) {
                                   printf("*** ERROR: exec failed\n");
                                   exit(1);
                              }
                         }
                         /* Input from a file when argv contains < */
                         if(singleLeftBracketPipeTwo){
                              fileDescriptorTwo = open(fileNameTwo, O_RDONLY, S_IRUSR | S_IWUSR);
                              if (fileDescriptorTwo == -1) {
                                   perror("ERROR opening the file:");
                                   return -1;
                              }
                              //fileDescriptorTwo remains open after dup2 is called
                              //STDIN_FILENO now points to fileDescriptorTwo
                              if(dup2(fileDescriptorTwo, STDIN_FILENO) < 0){
                                   perror("ERROR dup2 failed\n");
                              }
                              /* COMMAND LINE ARGUMENTS */
                              //Child process terminates after completion
                              if (execvp(*rightArgv, rightArgv) < 0) {
                                   printf("*** ERROR: exec failed\n");
                                   exit(1);
                              }
                         }
                         else{
                              /* COMMAND LINE ARGUMENTS */
                              //Child process terminates after completion
                              if (execvp(*rightArgv, rightArgv) < 0) {
                                   printf("*** ERROR: exec failed\n");
                                   exit(1);
                              }
                         }
                         exit(0);
                    }

               }
               /* First Child Process */
               //execute this when token has a pipe
               if(hasPipe){
                    //fd[1]- The write end of the pipe is the source file descriptor.
                    //This remains open after the call to dup2
                    //STDOUT_FILENO- The destination file descriptor.
                    //This file descriptor will point to the same file as fd[1] after this call returns.
                    dup2(fd[1], STDOUT_FILENO);
                    //close other end of the pipe
                    close(fd[0]);

                    fileDescriptor = open(fileName, O_RDONLY, S_IRUSR | S_IWUSR);
                    dup2(fileDescriptor, STDIN_FILENO);
                    if (execvp(*leftArgv, leftArgv) < 0) {
                         printf("*** ERROR: exec failed\n");
                         exit(1);
                    }
               }
               /* First Child Process */
               //execute this when token does not have a pipe
               else{
                    /* Output to a file when argv contains > */
                    //If file exists it'll be replaced.
                    if(singleRightBracket){
                         fileDescriptor = open(fileName, O_WRONLY | O_CREAT | O_TRUNC);
                         if (fileDescriptor == -1) {
                              perror("ERROR opening the file:");
                              return -1;
                         }
                         //fileDescriptor remains open after dup2 is called
                         //STDOUT_FILENO now points to fileDescriptor
                         dup2(fileDescriptor, STDOUT_FILENO);
                         /* COMMAND LINE ARGUMENTS */
                         //Child process terminates after completion
                         if (execvp(*myargv, myargv) < 0) {
                              printf("*** ERROR: exec failed\n");
                              exit(1);
                         }
                    }
                    /* Output to a file when argv contains >> */
                    //If file exists it'll be appended.
                    if(doubleRightBracket){
                         fileDescriptor = open(fileName, O_WRONLY | O_CREAT | O_APPEND);
                         if (fileDescriptor == -1) {
                              perror("ERROR opening the file:");
                              return -1;
                         }
                         //fileDescriptor remains open after dup2 is called
                         //STDOUT_FILENO now points to fileDescriptor
                         dup2(fileDescriptor, STDOUT_FILENO);
                         /* COMMAND LINE ARGUMENTS */
                         //Child process terminates after completion
                         if (execvp(*myargv, myargv) < 0) {
                              printf("*** ERROR: exec failed\n");
                              exit(1);
                         }
                    }
                    /* Input from a file when argv contains < */
                    if(singleLeftBracket){
                         fileDescriptor = open(fileName, O_RDONLY);
                         if (fileDescriptor == -1) {
                              perror("ERROR opening the file:");
                              return -1;
                         }
                         //fileDescriptor remains open after dup2 is called
                         //STDIN_FILENO now points to fileDescriptor
                         dup2(fileDescriptor, STDIN_FILENO);
                         /* COMMAND LINE ARGUMENTS */
                         //Child process terminates after completion
                         if (execvp(*myargv, myargv) < 0) {
                              printf("*** ERROR: exec failed\n");
                              exit(1);
                         }
                    }
                    else{
                         /* COMMAND LINE ARGUMENTS */
                         //Child process terminates after completion
                         if (execvp(*myargv, myargv) < 0) {
                              printf("*** ERROR: exec failed\n");
                              exit(1);
                         }
                    }

                    exit(0);
               }
          }

          /* Parent Process */
          else{
               //If token does not contain ampersand, wait for child process
               //If token does contain ampersand, do not wait for child process
               if(!ampersand){
                    waitpid(pid, &status, WUNTRACED);
               }
          }


     /* Reset */
     index = 0;
     leftIndex = 0;
     rightIndex = 0;
     myargc = 1;
     leftArgc = 0;
     rightArgc = 0;
     while(strcmp(buffer, "\n") == 0);
     singleRightBracket = false;
     singleRightBracketPipeOne = false;
     singleRightBracketPipeTwo = false;
     doubleRightBracket = false;
     doubleRightBracketPipeOne = false;
     doubleRightBracketPipeTwo = false;
     singleLeftBracket = false;
     singleLeftBracketPipeOne = false;
     singleLeftBracketPipeTwo = false;
     hasPipe = false;
     ampersand = false;

     }
     return 0;
}
