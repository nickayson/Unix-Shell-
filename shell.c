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
    while(char_args[i] != '\0')
    {
        if(!strncmp(char_args[i], "|", 1))      //checks to see if | was inputted
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

    char** redirectto = malloc(2 * sizeof(char*));

    for(int i = 0; i < 2; i++)
    {
        redirectto[i] = malloc(BUFSIZ * sizeof(char));
    }

    redirectto[0] = "";
    redirectto[1] = "";

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
            redirectto[0] = "o";
            redirectto[1] = p;
            return redirectto;
        } 
        if(!strncmp(p, "<", 1))
        {
            p = strtok(NULL, breakchars);
            redirectto[0] = "i";
            redirectto[1] = p;
            return redirectto;
        }
        char_args[num_args+1] = p;
    }
    return redirectto;
}   // end of parse function
//=========================================================================================
int main(int argc, const char* argv[])
{
    char input [BUFSIZ];            //given
    char last_command [BUFSIZ]; 
    int pipefd[2];              // pipe file

    // clear buffers
    memset(input, 0, BUFSIZ * sizeof(char));
    memset(last_command,   0, BUFSIZ * sizeof(char));   //given
    bool finished = false; //=0 also given

    while(!finished)
    {
        printf("osh> ");            //given
        fflush(stdout); 

        bool waitone = true;
        // read into input
        if (fgets(input, BUFSIZ, stdin)==NULL)
        {
            fprintf(stderr,"no command entered\n");
            exit(1);
        }

        input[strlen(input) - 1] = '\0';                // replace newline with null
        printf("input was: \n'%s'\n", input);

        if(strncmp(input, "!!", 2))                 // history
        {
            strcpy(last_command, input);
        } 
        if(strncmp(input, "exit", 4) == 0)          //get out of osh
        {
            return 0;
        }
        char* offset = strstr(input, "&");
        if(offset != NULL)
        {
            *offset = ' '; 
            waitone = false;
        } 
        //fork function to create child begins here
        pid_t pid = fork();
        int valuechild = 0;
        if(pid < valuechild)
        {
            fprintf(stderr, "fork failed...\n");
            exit(1); // exit
        }
        else if(pid != valuechild)      //parent
        {   
            if(waitone)
            {
                wait(NULL);         // wait if used with '&'
                wait(NULL);
            }
        }  
        else    //child
        {
            int history = 0;
            char* char_args[BUFSIZ];
            memset(char_args, 0, BUFSIZ * sizeof(char));

            // if we use '!!' we want to read from last_command for child now
            if(!strncmp(input, "!!", 2))
            {
                history = 1;
            }
            char** redirect = parse( (history ? last_command : input), char_args); // redirects to the instructions in parse
            // no command entered before history 
            if(history && last_command[0] == '\0')
            {
                printf("No command used.\n");
                exit(1);
            } 
            if(!strncmp(redirect[0], "o", 1))       // output redirect for the output
            {
                printf("Output saved to the file./%s\n", redirect[1]);
                int file = open(redirect[1], O_TRUNC | O_CREAT | O_RDWR);
                dup2(file, STDOUT_FILENO);        // redirect stdout to file 
            }
            if(!strncmp(redirect[0], "i", 1))      //redirect for the read
            { 
                printf("Reading from file: ./%s\n", redirect[1]);
                int file = open(redirect[1], O_RDONLY);   
                memset(input, 0, BUFSIZ * sizeof(char));
                memset(char_args, 0, BUFSIZ * sizeof(char));    //buffer reset
                read(file, input,  BUFSIZ * sizeof(char));
                input[strlen(input) - 1]  = '\0';
                parse(input, char_args);
            }
                execvp(char_args[0], char_args);                                //The things args array will be passed to
                exit(0);
        }  
    }
    printf("osh exited\n");
    printf("program finished\n");
    return 0;    
} 
//end of main
//===================================================================================================================