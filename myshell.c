/*
    Simplified Linux Shell (MyShell) 
*/

// Note: Necessary header files are included
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/stat.h> // For constants that are required in open/read/write/close syscalls
#include <sys/wait.h> // For wait() - suppress warning messages
#include <fcntl.h> // For open/read/write/close syscalls

// Assume that each command line has at most 256 characters (including NULL)
#define MAX_CMDLINE_LEN 256

// Assume that we have at most 8 arguments
#define MAX_ARGUMENTS 8

// Assume that we only need to support 2 types of space characters: 
// " " (space) and "\t" (tab)
#define SPACE_CHARS " \t"

// The pipe character
#define PIPE_CHAR "|"

// Assume that we only have at most 8 pipe segements, 
// and each segment has at most 256 characters
#define MAX_PIPE_SEGMENTS 8

// Assume that we have at most 8 arguments for each segment
//
// We also need to add an extra NULL item to be used in execvp
//
// Thus: 8 + 1 = 9
//
// Example: 
//   echo a1 a2 a3 a4 a5 a6 a7 
//
// execvp system call needs to store an extra NULL to represent the end of the parameter list
//
//   char *arguments[MAX_ARGUMENTS_PER_SEGMENT]; 
//
//   strings stored in the array: echo a1 a2 a3 a4 a5 a6 a7 NULL
//
#define MAX_ARGUMENTS_PER_SEGMENT 9

// Define the  Standard file descriptors here
#define STDIN_FILENO    0       // Standard input
#define STDOUT_FILENO   1       // Standard output 


 
// This function will be invoked by main()
void process_cmd(char *cmdline);

// read_tokens function is given
// This function helps you parse the command line
// Note: Before calling execvp, please remember to add NULL as the last item 
void read_tokens(char **argv, char *line, int *numTokens, char *token);

// Here is an example code that illustrates how to use the read_tokens function
// int main() {
//     char *pipe_segments[MAX_PIPE_SEGMENTS]; // character array buffer to store the pipe segements
//     int num_pipe_segments; // an output integer to store the number of pipe segment parsed by this function
//     char cmdline[MAX_CMDLINE_LEN]; // the input argument of the process_cmd function
//     int i, j;
//     char *arguments[MAX_ARGUMENTS_PER_SEGMENT] = {NULL}; 
//     int num_arguments;
//     strcpy(cmdline, "ls | sort -r | sort | sort -r | sort | sort -r | sort | sort -r");
//     read_tokens(pipe_segments, cmdline, &num_pipe_segments, PIPE_CHAR);
//     for (i=0; i< num_pipe_segments; i++) {
//         printf("%d : %s\n", i, pipe_segments[i] );    
//         read_tokens(arguments, pipe_segments[i], &num_arguments, SPACE_CHARS);
//         for (j=0; j<num_arguments; j++) {
//             printf("\t%d : %s\n", j, arguments[j]);
//         }
//     }
//     return 0;
// }


/* The main function implementation */
int main()
{
    char cmdline[MAX_CMDLINE_LEN];
    fgets(cmdline, MAX_CMDLINE_LEN, stdin);
    process_cmd(cmdline);
    return 0;
}

void process_cmd(char *cmdline)
{
    //printf("%s\n", cmdline);   // You can try to write: printf("%s\n", cmdline); to check the content of cmdline

    char *pipe_segments[MAX_PIPE_SEGMENTS]; // character array buffer to store the pipe segements
    int num_pipe_segments; // an output integer to store the number of pipe segment parsed by this function
    int i, j, flag = 0;
    char *arguments[MAX_ARGUMENTS_PER_SEGMENT] = {NULL}; 
    char *in_arguments[MAX_ARGUMENTS_PER_SEGMENT] = {NULL};
    int num_arguments;
    //int stdout_fd = dup(1);
    read_tokens(pipe_segments, cmdline, &num_pipe_segments, PIPE_CHAR);

    if(num_pipe_segments > 1) {
        int pdfs[2], k;
        int prev_pipe_out = 0;
        for(k = 0; k < num_pipe_segments; k++){
            //Get arguments
            //printf("pipe %d executed\n", k);
            read_tokens(arguments, pipe_segments[k], &num_arguments, SPACE_CHARS);
            arguments[num_arguments] = NULL;
            pipe(pdfs);
            pid_t pid = fork();
            if(pid == 0){

                //get input from previos pipe
                if(prev_pipe_out != 0){
                    dup2(prev_pipe_out, STDIN_FILENO);
                    //close(pdfs[1]);  //Doesnt work 
                    close(prev_pipe_out);
                }

                // If last pipe different case
                if(k != num_pipe_segments-1){
                    dup2(pdfs[1], STDOUT_FILENO);
                    //close(pdfs[0]);  //Same reason
                    close(pdfs[1]);
                }

                //Close rear end
                close(pdfs[0]);
                execvp(arguments[0], arguments);

            } else if(pid > 0) {
                //output is recieved by parent process
                //wait for child
                wait(NULL);
                close(pdfs[1]);
                prev_pipe_out = pdfs[0];
            }
            else{
                break;;
            }
        }
        
    }

    else if(num_pipe_segments == 1){
        for (i=0; i< num_pipe_segments; i++){
            read_tokens(arguments, pipe_segments[num_pipe_segments-1], &num_arguments, SPACE_CHARS);
            for (j=0; j<num_arguments; j++) {
                //printf("\t%d : %s\n", j, arguments[j]);
                if(flag == 0){
                    in_arguments[j] = arguments[j];
                }
                if(strcmp(arguments[j], ">") == 0){
                    //printf("\t%d : %s\n", j+1, arguments[j+1]);
                    flag = 1;
                    //printf("Input detected\n");
                    int fd_out = open(arguments[j+1],
                                O_CREAT | O_WRONLY, //flags
                                S_IRUSR | S_IWUSR); //User Permission: 600
                    close(STDOUT_FILENO); // Close stdout
                    dup(fd_out); //Replace stdout using the new file descriptor ID
                    in_arguments[j] = NULL;

                }
                else if(strcmp(arguments[j], "<") == 0){
                    //printf("\t%d : %s\n", j+1, arguments[j+1]);
                    //printf("Output detected\n");
                    flag = 1;
                    int fd_in = open(arguments[j+1],
                                O_RDONLY, //flags
                                S_IRUSR | S_IWUSR); // User Permission: 600
                    close(STDIN_FILENO); // Close stdout
                    dup(fd_in); // Replace stdout using the new file descriptor ID
                    in_arguments[j] = NULL;
                }   
            }
            if(flag != 0){
                execvp(pipe_segments[num_pipe_segments-1], in_arguments);
                //fflush(stdout);
            }
            else{
                execvp(pipe_segments[i], arguments);
                //fflush(stdout);
            }
        }
    }
}

// Implementation of read_tokens function
void read_tokens(char **argv, char *line, int *numTokens, char *delimiter)
{
    int argc = 0;
    char *token = strtok(line, delimiter);
    while (token != NULL)
    {
        argv[argc++] = token;
        token = strtok(NULL, delimiter);
    }
    *numTokens = argc;
}
