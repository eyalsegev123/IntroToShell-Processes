#include <stdio.h> // C standard
#include <unistd.h> // execv, fork ...
#include <linux/limits.h> // PATH MAX
#include <stdlib.h> // maloc
#include <string.h> // strcmp
#include <sys/types.h> // data types in system call
#include <sys/wait.h> // waitpd
#include <errno.h> // errno
#include <signal.h> //SIG
#include "LineParser.h"
#include <stdbool.h>
#include <ctype.h>

#define TERMINATED  -1
#define RUNNING 1
#define SUSPENDED 0

#ifndef MAX_INPUT
#define MAX_INPUT 2048
#endif


typedef struct process{
        cmdLine* cmd;                         /* the parsed command line*/
        pid_t pid; 		                  /* the process id that is running the command*/
        int status;                           /* status of the process: RUNNING/SUSPENDED/TERMINATED */
        struct process *next;	                  /* next process in chain */
} process;

int handleCDcommand(cmdLine * pCmdLine , bool debug);
void handle_signal_commands(cmdLine *pCmdLine , bool debug, process** process_list);
void handleRedirection(cmdLine * pCmdLine);
void show_history(); 
char* get_command_from_history(int number); 
int is_numeric(const char *str);
int addToHistory(char* command);   
void clean(char* fileName);
void addProcess(process** process_list, cmdLine* cmd, pid_t pid);
void printProcessList(process** process_list);
void freeProcessList(process* process_list);
void updateProcessStatus(process* process_list, int pid, int status);
void updateProcessList(process** process_list);

void addProcess(process** process_list, cmdLine* cmd, pid_t pid) {
    process* newProcess = malloc(sizeof(process));
    newProcess->cmd = cmd;
    newProcess->pid = pid;
    newProcess->status = RUNNING;
    newProcess->next = *process_list;
    *process_list = newProcess;
}

void printProcessList(process** process_list) {
    printf("PID\tCommand\t\tSTATUS\n");
    process* current = *process_list;
    while (current != NULL) {
        printf("%d\t%s\t%s\n", current->pid, current->cmd->arguments[0], 
               (current->status == TERMINATED ? "Terminated" : 
                current->status == RUNNING ? "Running" : "Suspended"));
        current = current->next;
    }
}

void freeProcessList(process* process_list) {
    process* current = process_list;
    while (current != NULL) {
        process *temp = current;
        current = current->next;
        freeCmdLines(temp->cmd);
        free(temp);
    }
}

void updateProcessList(process** process_list) {
    process *current = *process_list;
    process *prev = NULL;
    int status;

    while (current != NULL) {
        pid_t result = waitpid(current->pid, &status, WNOHANG);
        if (result == -1 || WIFEXITED(status)) {
            // Process is terminated
            current->status = TERMINATED;
            printf("PID %d: %s Terminated\n", current->pid, current->cmd->arguments[0]);
            process *to_free = current;
            if (prev == NULL) {
                *process_list = current->next;
                current = *process_list;
            } else {
                prev->next = current->next;
                current = current->next;
            }
            freeCmdLines(to_free->cmd);
            free(to_free);
        } 
        else {
            // Process is still running or suspended
            prev = current;
            current = current->next;
        }
    }
}

void updateProcessStatus(process* process_list, int pid, int status) {
    process *current = process_list;
    while (current != NULL) {
        if (current->pid == pid) {
            current->status = status;
            break;
        }
        current = current->next;
    }
}

int handleCDcommand(cmdLine * pCmdLine , bool debug){
    if (strcmp(pCmdLine->arguments[0], "cd") == 0) 
    {
        // Handle the internal "cd" command
        if (pCmdLine->argCount < 2) {
            if(debug)
                {fprintf(stderr, "cd: no directory was written\n");}
        } else {
             int cdCheck = chdir(pCmdLine->arguments[1]);
             if (cdCheck == 0) { return 0;}
             else
             {
                if(debug)
                    {perror("cd failed");}
             }
            }   
            return 1; 
    } 
    return 0; 
}

int is_numeric(const char *str) {
    // Check if every character in the string is a digit
    while (*str) {
        if (!isdigit(*str)) {
            return 0; // Not a number
        }
        str++;
    }
    return 1; // All characters are digits
}

void handleRedirection(cmdLine * pCmdLine){
    if (!freopen(pCmdLine->inputRedirect, "r", stdin)) { //handling input redirectoin
        perror("Failed to redirect stdin");
        exit(1);
    }

    // Redirect stdout to output.txt
    if (!freopen(pCmdLine->outputRedirect, "w", stdout)) { //handling output redirectoin
        perror("Failed to redirect stdout");
        exit(1);
    }  
}

void handle_signal_commands(cmdLine *pCmdLine , bool debug, process** process_list) {
    if (pCmdLine->argCount != 2) {
        if(debug)
            {fprintf(stderr, "Usage: %s <process id>\n", pCmdLine->arguments[0]);}
        return;
    }

    int pid = atoi(pCmdLine->arguments[1]);
    if (pid <= 0) {
        if(debug)
            {fprintf(stderr, "Invalid process ID: %s\n", pCmdLine->arguments[1]);}
        return;
    }

    if (strcmp(pCmdLine->arguments[0], "alarm") == 0) {
        if (kill(pid, SIGCONT) == 0) {
            if(debug)
                {printf("Sent SIGCONT to process %d\n", pid);}
            updateProcessStatus(*process_list, pid, RUNNING);
        } else {
            if(debug)
                {perror("Failed to send SIGCONT");}
        }
    } 
    else if (strcmp(pCmdLine->arguments[0], "blast") == 0) {
        if (kill(pid, SIGKILL) == 0) {
            if(debug)
                {printf("Sent SIGKILL to process %d\n", pid);}
            updateProcessStatus(*process_list, pid, TERMINATED);
        } else {
            if(debug)
                {perror("Failed to send SIGKILL");}
        }
    }
    else if (strcmp(pCmdLine->arguments[0], "sleep") == 0) {
        // Send SIGTSTP to suspend a running process
        if (kill(pid, SIGTSTP) == 0) {
            if (debug) {
                printf("Sent SIGTSTP to process %d\n", pid);
            }
            updateProcessStatus(*process_list, pid, SUSPENDED);
        } else {
            if (debug) {
                perror("Failed to send SIGTSTP");
            }
        }
    }
}

void show_history() {
    FILE *file = fopen("shellHistory", "r");
    if (file == NULL) {
        perror("Error opening history file");
        return;
    }
    char line[256];
    int line_number = 1;
    while (fgets(line, sizeof(line), file)) {
        printf("%d %s", line_number++, line);
    }
    fclose(file);
}

char* get_command_from_history(int number) {
    FILE *file = fopen("shellHistory", "r");
    if (file == NULL) {
        perror("Error opening history file");
        return NULL;
    }
    char line[256];
    int line_number = 1;
    while (fgets(line, sizeof(line), file)) {
        if (line_number == number) {
            fclose(file);
            return strdup(line);
        }
        line_number++;
    }
    fclose(file);
    return NULL;
}

int addToHistory(char* command){
    FILE* shellHistory = fopen("shellHistory", "a");
    if(shellHistory == NULL)
    {
        perror("Error opening history file");
        return EXIT_FAILURE;
    }
    fprintf(shellHistory, "%s", command);
    fclose(shellHistory);
    return 1;
}

void clean(char* fileName){
    // Open the file in write mode
    FILE *fp = fopen(fileName, "w");
    if (fp == NULL) {
        perror("Error opening file");
    }
    else
    {
        // Truncate the file to zero length
        if (truncate(fileName, 0) == -1) {
            perror("Error truncating file");
            fclose(fp);
        }
        // Close the file
        fclose(fp);
    }
}

void execute(cmdLine *pCmdLine, bool debug, process** process_list) {
    updateProcessList(process_list);
    // Handle built-in commands and special cases first
    if (handleCDcommand(pCmdLine, debug) == 1) {
        return;
    } 
    else if (strcmp(pCmdLine->arguments[0], "history") == 0) {
        show_history();
        return;
    } 
    else if (pCmdLine->arguments[0][0] == '!') {
        if (is_numeric(pCmdLine->arguments[0] + 1)) {
            int number = atoi(pCmdLine->arguments[0] + 1);
            cmdLine *newCommand = parseCmdLines(get_command_from_history(number));
            execute(newCommand, debug, process_list);
            freeCmdLines(newCommand); // Free allocated memory for cmdLine structure
        } else {
            perror("Invalid command");
        }
        return;
    }
    else if(strcmp(pCmdLine->arguments[0], "procs") == 0)
    {
        printProcessList(process_list);
        return;
    }
    else if (strcmp(pCmdLine->arguments[0], "alarm") == 0 || 
            strcmp(pCmdLine->arguments[0], "blast") == 0 || 
            strcmp(pCmdLine->arguments[0], "sleep") == 0) {
        // Handle signal commands
        handle_signal_commands(pCmdLine, debug , process_list);
        return;
    }
    

    // Check if the command line contains a single pipe
    if (pCmdLine->next != NULL && pCmdLine->next->next == NULL) {
        int pipefd[2];
        
        // Create a pipe
        if (pipe(pipefd) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }

        // Fork the first child process
        pid_t pid1 = fork();
        if (pid1 == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid1 == 0) {
            // Child process for the first command (left-hand side)
            close(pipefd[0]); // Close unused read end of the pipe
            handleRedirection(pCmdLine); // Handle I/O redirection for the first command
            dup2(pipefd[1], STDOUT_FILENO); // Redirect stdout to the write end of the pipe
            close(pipefd[1]); // Close the write end of the pipe
            execvp(pCmdLine->arguments[0], pCmdLine->arguments); // Execute the first command
            perror("execvp"); // Print error if execvp fails
            _exit(EXIT_FAILURE); // Terminate the child process
        }

        // Fork the second child process
        pid_t pid2 = fork();
        if (pid2 == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid2 == 0) {
            // Child process for the second command (right-hand side)
            close(pipefd[1]); // Close unused write end of the pipe
            handleRedirection(pCmdLine->next); // Handle I/O redirection for the second command
            dup2(pipefd[0], STDIN_FILENO); // Redirect stdin to the read end of the pipe
            close(pipefd[0]); // Close the read end of the pipe
            execvp(pCmdLine->next->arguments[0], pCmdLine->next->arguments); // Execute the second command
            perror("execvp"); // Print error if execvp fails
            _exit(EXIT_FAILURE); // Terminate the child process
        }
        addProcess(process_list, pCmdLine, pid1);
        addProcess(process_list, pCmdLine, pid2);

        // Close both ends of the pipe in the parent process
        close(pipefd[0]);
        close(pipefd[1]);

        // Wait for both child processes to complete
        waitpid(pid1, NULL, 0);
        updateProcessStatus(*process_list, pid1, TERMINATED);
        waitpid(pid2, NULL, 0);
        updateProcessStatus(*process_list, pid2, TERMINATED);

    } else {
        // Execute a single command (no pipeline)
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        else if (pid == 0) {
            // Child process
            handleRedirection(pCmdLine); // Handle I/O redirection for the command
            execvp(pCmdLine->arguments[0], pCmdLine->arguments); // Execute the command
            perror("execvp"); // Print error if execvp fails
            _exit(EXIT_FAILURE); // Terminate the child process
        } 
        else {
            // Parent process
            if (debug) {
                fprintf(stderr, "PID: %d\n", pid);
                fprintf(stderr, "Executing command: %s\n", pCmdLine->arguments[0]);
            }
            addProcess(process_list, pCmdLine, pid);
            if (pCmdLine->blocking == 1) {
                // Wait for the child process to complete if blocking is enabled
                waitpid(pid, NULL, 0);
                updateProcessStatus(*process_list, pid, TERMINATED);
            }
        }
    }
}


int main(int argc, char **argv){
    process* process_list_head = NULL;
    process** process_list = &process_list_head;
    bool debug = false;
    char cwd[PATH_MAX];
    char input[MAX_INPUT];
    if(strcmp(argv[argc-1] , "-d"))
    {
        debug = true;
    }
    clean("shellHistory");
    while(1){
        if (getcwd(cwd, sizeof(cwd)) != NULL) 
        {
            printf("Current working directory: %s\n", cwd);
        } 
        else 
        {
            if(debug)
                {perror("getcwd() error");}
        }
        printf("Enter input here:\n");
        if (fgets(input, sizeof(input), stdin) != NULL) 
        {
            printf("You entered: %s", input);
        }
        else 
        {
            if(debug)
                {perror("fgets() error");}
            return 1;
        }
        if(strcmp(input , "quit") == 0)
        {
            break;
        }
        cmdLine* command = parseCmdLines(input); //parses the input into a cmdLine structure    
        addToHistory(input);
        execute(command , debug, process_list); //fork a new process and execute the command   
    }
    freeProcessList(*process_list);
    return 0;
}
