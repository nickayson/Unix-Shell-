// Nicholas Ayson
// Due on March 28, 2021
// Unix Shell project

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_LINE	100 //100 chars per line, per command 
#define READ_END	0
#define WRITE_END	1

//Function that calls for user input
void Input(char *args[], bool *Ampsign, int *numOfElem)
{
	char input[MAX_LINE];
    int lengthinput = read(STDIN_FILENO, input, 80);
    char delimiter[] = " ";

	// given
	if (lengthinput > 0 && input[lengthinput - 1] == '\n')
    {
        input[lengthinput - 1] = '\0';
    }
    if (strcmp(input, "!!") == 0)
	{	
        if (*numOfElem == 0)
		{
            printf("No commands in history.\n");
        }
		return;
    }
	if (strcmp(input, "exit") == 0)
	{
		exit(0);
	}

	for(int i = 0; i < *numOfElem; i++)
	{	
		args[i] = NULL;
	}
	
    *numOfElem = 0;
    *Ampsign = false;

	char *ptr = strtok(input, delimiter);

	//Breaks the input to use
	while(ptr != NULL)
	{
		//If an ampersand
		if (ptr[0] == '&')
		{
            *Ampsign = true;				//Flag if program detects an ampersand
            ptr = strtok(NULL, delimiter);
        }

		args[*numOfElem] = strdup(ptr);
		*numOfElem += 1;						//Increase number of elements
		ptr = strtok(NULL, delimiter);			//set ptr to NULL
	}

	args[*numOfElem] = NULL;
}
//end of function call for input
// Main Program
int main(void)
{
	char *args[MAX_LINE/2 + 1];	
	bool Ampsign = false;		//0 if no amp
	int numOfElem = 0;			// Number of Elements

	bool Pipe = false;		//Flag to indicate we are using a pipe
    bool finished = false;  
	pid_t pid;
		
    while (!finished)
	{   
		Pipe = false;
        printf("osh>");//given
        fflush(stdout);

		 Input(args, &Ampsign, &numOfElem);
		 pid = fork();

		//If pid < 0 then an error has occured
		if(pid < 0)
		{
			fprintf(stderr, "Fork Failed"); 
			return 1;
		}

		//If pid == 0 then child created successfully 
		else if(pid == 0)
		{
			//If the array is not empty
			if(numOfElem != 0)
			{
				int file;						//File name
				bool redirectInput = false;		//indicate intput file
				bool redirectOutput = false; 	//indicate output outfile

				// check for input '<', output '>', or a '|'
				for (int i = 0; i < numOfElem; i++)
				{
					//if '<' input
					if(strcmp(args[i], "<") == 0)
					{
						file = open(args[i + 1], O_RDONLY);
						printf("input saved to this file.\n");
						if (args[i + 1]  == NULL || file == -1)
						{
                           	printf("Invalid Command!\n");
                           	return 1;
                       	}

						dup2(file, STDIN_FILENO);	//Any writes in standard output will be sent to file
						redirectInput = true;	
						args[i] = NULL;				
                 		args[i + 1] = NULL;	

						break;
					}
					//else if '>' output
					else if(strcmp(args[i], ">") == 0)
					{
						file = open(args[i + 1], O_WRONLY | O_CREAT);	//Will create and write to file
						printf("Output saved to this file.\n");
						if (args[i + 1] == NULL || file == -1)
						{
                           	printf("Invalid Command!\n");
                      	 	return 1;
                    	}

						dup2(file, STDOUT_FILENO);	//Any writes in standard output will be sent to file
						redirectOutput = true;
						args[i] = NULL;				//Sets "<" to NULL
                   		args[i + 1] = NULL;	

						break;
					}
					//pipe
					else if(strcmp(args[i], "|") == 0)
					{
						Pipe = true;
                    	int fd[2];
                   		if (pipe(fd) == -1)
						{
							fprintf(stderr,"Pipe failed");
							return 1;
						}
						
						//Create two arrays for the first and second set of commands
                    	char *arr1[i + 1];
                    	char *arr2[numOfElem - i + 1];

						//Fill first array
                    	for (int j = 0; j < i; j++)
						{
                        	arr1[j] = args[j];
                    	}					
                    	arr1[i] = NULL;

						//Fill second array
                    	for (int j = 0; j < numOfElem - i - 1; j++)
						{
                       	 arr2[j] = args[j + i + 1];
                    	}
                    	arr2[numOfElem - i - 1] = NULL;

						//Now fork a child process
                    	pid_t pidPipe = fork();
						
						//Error handling
						if (pidPipe < 0)
						{
							fprintf(stderr, "Fork failed.");
							return 1;
						}
                    	else if (pidPipe > 0)
						{
							//Wait for child process to complete
                       		wait(NULL);

							//Close unused pipe
                       		close(fd[WRITE_END]);
                        	dup2(fd[READ_END], STDIN_FILENO);
                        	close(fd[READ_END]);

							//Error
                        	if (execvp(arr2[0], arr2) == -1)
							{
                            	printf("Invalid Command!\n");
                            	return 1;
                        	}
                    	}
						else
						{
							//Close unused pipe
                        	close(fd[READ_END]);
                      		dup2(fd[WRITE_END], STDOUT_FILENO);
                        	close(fd[WRITE_END]);

							//Error
                        	if (execvp(arr1[0], arr1) == -1)
							{
                           		printf("Invalid Command!\n");
                            	return 1;
                        	}
                       	}
                    	close(fd[READ_END]);
                    	close(fd[WRITE_END]);
						
                    	break;
					}
				}

				//If no redirects
                if (Pipe == false)
				{
					//Error
                    if (execvp(args[0], args) == -1)
					{
                        printf("Invalid Command!\n");
                        return 1;
                    }
                }

				//Close input and output
                if (redirectInput == false) 
				{
                    close(STDIN_FILENO);

                }
				else if (redirectOutput == false)
				{
                    close(STDOUT_FILENO);
                }
                close(file);
			}
		}
		//Parent  
		else
		{
			//If there is no ampersand '&', then parent will wait
			if(Ampsign == false)
			{
				wait(NULL);
			}
		}
    }
	return 0;
}
//main finished