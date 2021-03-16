// Nicholas Ayson
// Due on March 25, 2021
// Unix Shell project

//===============================================================================================================================================================================


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#define READ_END 0 
#define WRITE_END 1

int piperhs(char** cargs);

//start of parse function
char** tokparse(char* s, char* cargs[])
{
    const char breakchars[2] = {' ','\t'};
    char* p;
    int num_args = 0;

    p = strtok(s, breakchars);
    cargs[0] = p;

    char** REDIR = malloc(2 * sizeof(char*));

    for(int i = 0; i < 2; i++)
    {
        REDIR[i] = malloc(BUFSIZ * sizeof(char));
    }

    REDIR[0] = "";
    REDIR[1] = "";

    while(p != NULL)
    {
        // add p to cargs
        p = strtok(NULL, breakchars);

        if(p == NULL) 
        {
            break;
        }
        if(!strncmp(p, ">", 1))
        {
            p = strtok(NULL, breakchars);
            REDIR[0] = "o";
            REDIR[1] = p;
            return REDIR;
        } 
        else if(!strncmp(p, "<", 1))
        {
            p = strtok(NULL, breakchars);
            REDIR[0] = "i";
            REDIR[1] = p;
            return REDIR;
        }
        else if(!strncmp(p, "|", 1))
        {   // pipe
            REDIR[0] = "p";        
        }
        cargs[++num_args] = p;
    }
    return REDIR;
}   // end of parse function

int main(int argc, const char* argv[])
{
    char s [BUFSIZ];
    char MRU   [BUFSIZ]; // most-recently used cache
    int pipefd[2];      // file descriptor

    // clear buffers
    memset(s, 0, BUFSIZ * sizeof(char));
    memset(MRU,   0, BUFSIZ * sizeof(char));
   

    while(1)
    {
        printf("osh> ");
        fflush(stdout); 

        // read into s
        fgets(s, BUFSIZ, stdin);
        s[strlen(s) - 1] = '\0';                // replace newline with null

        // edge cases
        if(strncmp(s, "exit", 4) == 0)          //get out of osh
        {
            return 0;
        }
        if(strncmp(s, "!!", 2))                 // history
        {
            strcpy(MRU, s);
        } 

        // wait for '&'
        bool waitx = true;
        char* offset = strstr(s, "&");
        if(offset != NULL)
        {
            *offset = ' '; 
            waitx = false;
        }

        pid_t pid = fork();
        if(pid < 0)
        {   // failed to create child 
            fprintf(stderr, "fork failed...\n");
            return -1; // exit
        }
        else if(pid != 0)
        {   // parent
            if(waitx){
                wait(NULL);         // wait if used with '&'
                wait(NULL);
            }
        }
        else
        {   // child
            char* cargs[BUFSIZ];
            memset(cargs, 0, BUFSIZ * sizeof(char));

            int history = 0;
            // if we use '!!' we want to read from MRU
            if(!strncmp(s, "!!", 2))
            {
                history = 1;
            }
            char** redirect = tokparse( (history ? MRU : s), cargs);
            // no command entered before history function
            if(history && MRU[0] == '\0')
            {
                printf("No recently used command.\n");
                exit(0);
            } 
            if(!strncmp(redirect[0], "o", 1))
            {   // output redirect
                printf("Output saved to the file./%s\n", redirect[1]);
                int fd = open(redirect[1], O_TRUNC | O_CREAT | O_RDWR);
                dup2(fd, STDOUT_FILENO); // redirect stdout to file descriptor 
            }
            else if(!strncmp(redirect[0], "i", 1))
            {   // s redirect
                printf("Reading from file: ./%s\n", redirect[1]);
                int fd = open(redirect[1], O_RDONLY);
                memset(s, 0, BUFSIZ * sizeof(char));
                read(fd, s,  BUFSIZ * sizeof(char));
                memset(cargs, 0, BUFSIZ * sizeof(char));
                s[strlen(s) - 1]  = '\0';
                tokparse(s , cargs);
            }
            else if(!strncmp(redirect[0], "p", 1))
            {   // found a pipe
                pid_t pidchild;                 //child
                int rhsoffset = piperhs(cargs);
                cargs[rhsoffset] = "\0";
                int i = pipe(pipefd);
                if(i < 0)
                {
                    fprintf(stderr, "Pipe failed\n");
                    return 1;
                }

                char* lhs[BUFSIZ], *rhs[BUFSIZ];
                memset(lhs, 0, BUFSIZ*sizeof(char));                //buffers to zero
                memset(rhs, 0, BUFSIZ*sizeof(char));

                for(int i = 0; i < rhsoffset; i++)
                {
                    lhs[i] = cargs[i];
                }
                for(int i = 0; i < BUFSIZ; i++){
                    int ix = ix + rhsoffset + 1;
                    if(cargs[i] == 0) 
                    {
                        break;
                    }
                    rhs[i] = cargs[ix];
                }
                
                pidchild = fork();                      // create child to handle pipe's rhs
                if(pidchild < 0)
                {
                    fprintf(stderr, "fork failed\n");
                    return 1;
                }
                if(pidchild != 0)
                {   // parent process 
                    dup2(pipefd[WRITE_END], STDOUT_FILENO);         // duplicate stdout to write end of file descriptor
                    close(pipefd[WRITE_END]);                       // close write
                    execvp(lhs[0], lhs);
                    exit(0); 
                }
                else
                {   // child process
                    dup2(pipefd[READ_END], STDIN_FILENO);           // duplicate read end of pipe to stdin
                    close(pipefd[READ_END]);                        // close read 
                    execvp(rhs[0], rhs);                           
                    exit(0);
                }
                wait(NULL);
            }
            execvp(cargs[0], cargs);                                //The things args array will be passed to
            exit(0);  
        }
    }

    return 0;
}
//end of main

//Locate pipe char and return inside
int piperhs(char** cargs){
    int i = 0;
    while(cargs[i] != '\0'){
        if(!strncmp(cargs[i], "|", 1)) 
        {   // found pipe
            return i;                               // new cargs starting at offset
        }
        ++i;
    }
    return -1;
}// end of locate pipe