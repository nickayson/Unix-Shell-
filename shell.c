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

//===============================================================================================================
//Locate pipe char and return inside
//function for the right pipe
int piperhs(char** char_args)
{
    int i = 0;
    while(char_args[i] != '\0'){
        if(!strncmp(char_args[i], "|", 1)) 
        {   
            return i;                               // new char_args starting at offset
        }
        i++;
    }
    return -1;
}// end of locate pipe 
//===============================================================================================================================
//start of parse function
char** parse(char* s, char* char_args[])
{
    const char breakchars[2] = {' ','\t'};
    char* p;
    int num_args = 0;

    p = strtok(s, breakchars);
    char_args[0] = p;

    char** REDIR = malloc(2 * sizeof(char*));

    for(int i = 0; i < 2; i++)
    {
        REDIR[i] = malloc(BUFSIZ * sizeof(char));
    }

    REDIR[0] = "";
    REDIR[1] = "";

    while(p != NULL)
    {
        // add p to char_args
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
        char_args[++num_args] = p;
    }
    return REDIR;
}   // end of parse function

int main(int argc, const char* argv[])
{
    char s [BUFSIZ];
    char last_command   [BUFSIZ]; // most-recently used cache
    int pipefd[2];      // file descriptor

    // clear buffers
    memset(s, 0, BUFSIZ * sizeof(char));
    memset(last_command,   0, BUFSIZ * sizeof(char));


    while(1)
    {
        printf("osh> ");            //given
        fflush(stdout); 

        // read into s
        fgets(s, BUFSIZ, stdin);
        s[strlen(s) - 1] = '\0';                // replace newline with null

        if(strncmp(s, "exit", 4) == 0)          //get out of osh
        {
            return 0;
        }
        if(strncmp(s, "!!", 2))                 // history
        {
            strcpy(last_command, s);
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
            char* char_args[BUFSIZ];
            memset(char_args, 0, BUFSIZ * sizeof(char));

            int history = 0;
            // if we use '!!' we want to read from last_command
            if(!strncmp(s, "!!", 2))
            {
                history = 1;
            }
            char** redirect = parse( (history ? last_command : s), char_args);
            // no command entered before history function
            if(history && last_command[0] == '\0')
            {
                printf("No recently used command.\n");
                exit(0);
            } 
            if(!strncmp(redirect[0], "o", 1))
            {   // output redirect for the output
                printf("Output saved to the file./%s\n", redirect[1]);
                int fd = open(redirect[1], O_TRUNC | O_CREAT | O_RDWR);
                dup2(fd, STDOUT_FILENO); // redirect stdout to file descriptor 
            }
            else if(!strncmp(redirect[0], "i", 1))
            {   // s redirect for the read
                printf("Reading from file: ./%s\n", redirect[1]);
                int fd = open(redirect[1], O_RDONLY);   
                memset(s, 0, BUFSIZ * sizeof(char));
                read(fd, s,  BUFSIZ * sizeof(char));
                memset(char_args, 0, BUFSIZ * sizeof(char));
                s[strlen(s) - 1]  = '\0';
                parse(s , char_args);
            }
            else if(!strncmp(redirect[0], "p", 1))
            {   // found a pipe
                pid_t pidchild;                 //child
                int rhsoffset = piperhs(char_args);
                char_args[rhsoffset] = "\0";
                int i = pipe(pipefd);

                if(i < 0)
                {
                    fprintf(stderr, "Pipe failed\n");
                    return 1;
                }

                char* lhs[BUFSIZ], *rhs[BUFSIZ];        //left buffer and right buffer
                memset(lhs, 0, BUFSIZ*sizeof(char));                //buffers to zero
                memset(rhs, 0, BUFSIZ*sizeof(char));

                for(int i = 0; i < rhsoffset; i++)
                {
                    lhs[i] = char_args[i];
                }
                for(int i = 0; i < BUFSIZ; i++)
                {
                    int ix = ix + rhsoffset + 1;
                    if(char_args[i] == 0) 
                    {
                        break;
                    }
                    rhs[i] = char_args[ix];
                }

                pidchild = fork();                      // create child to handle pipe's rhs
                if(pidchild < 0)
                {
                    fprintf(stderr, "fork failed\n");
                    return 1;
                }
                if(pidchild != 0)
                {   // parent process 
                    dup2(pipefd[1], STDOUT_FILENO);         // duplicate stdout to write end of file descriptor
                    close(pipefd[1]);                       // close write
                    execvp(lhs[0], lhs);                    //execvp is used in child process
                    exit(0); 
                }
                else
                {   // child process
                    dup2(pipefd[0], STDIN_FILENO);           // duplicate read end of pipe to stdin
                    close(pipefd[0]);                        // close read 
                    execvp(rhs[0], rhs);                           
                    exit(0);
                }
                wait(NULL);
            }
            execvp(char_args[0], char_args);                                //The things args array will be passed to
            exit(0);  
        }
    }

    return 0;
}
//end of main
//===================================================================================================================