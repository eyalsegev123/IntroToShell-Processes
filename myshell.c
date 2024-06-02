#include <stdio.h>
#include <unistd.h>
#include <linux/limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include "LineParser.h"

#ifndef MAX_INPUT
#define MAX_INPUT 2048
#endif

int handleCDcommand(cmdLine * pCmdLine){
    if (strcmp(pCmdLine->arguments[0], "cd") == 0) 
    {
        // Handle the internal "cd" command
        if (pCmdLine->argCount < 2) {
            fprintf(stderr, "cd: no directory was written\n");
        } else if (chdir(pCmdLine->arguments[1]) != 0) {
            perror("cd failed");
        }
        return 0; //To go back to main and not make child process 
    }
    return 1;
}




void execute(cmdLine * pCmdLine) {
    
    if(handleCDcommand(pCmdLine) == 0) {return;}  
    else if (strcmp(pCmdLine->arguments[0], "alarm") == 0 || strcmp(pCmdLine->arguments[0], "blast") == 0) {
        // Handle the "alarm" and "blast" commands
        handle_signal_commands(pCmdLine);
        return;
    }
    
    pid_t pid = fork();
    if (pid == -1) 
    {
        perror("fork failed");
        exit(1);
    } 
    else if (pid == 0) 
    {
        // Child process
        handleRedirection(pCmdLine);
        execvp(pCmdLine->arguments[0], pCmdLine->arguments); //replace the current process image with a new process
        perror("execv failed");
        _exit(1); //terminate the child process
    } 
    else 
    {
        // Parent 
        fprintf(stderr, "PID: %d\n", pid);
        fprintf(stderr, "Executing command: %s\n", pCmdLine->arguments[0]);
        if (pCmdLine->blocking == 1) {
            // Wait for the child process to complete if blocking is 1
            waitpid(pid, NULL, 0);
        }
    }
}

void handle_signal_commands(cmdLine *pCmdLine) {
    if (pCmdLine->argCount != 2) {
        fprintf(stderr, "Usage: %s <process id>\n", pCmdLine->arguments[0]);
        return;
    }

    int pid = atoi(pCmdLine->arguments[1]);
    if (pid <= 0) {
        fprintf(stderr, "Invalid process ID: %s\n", pCmdLine->arguments[1]);
        return;
    }

    if (strcmp(pCmdLine->arguments[0], "alarm") == 0) {
        if (kill(pid, SIGCONT) == 0) {
            printf("Sent SIGCONT to process %d\n", pid);
        } else {
            perror("Failed to send SIGCONT");
        }
    } else if (strcmp(pCmdLine->arguments[0], "blast") == 0) {
        if (kill(pid, SIGKILL) == 0) {
            printf("Sent SIGKILL to process %d\n", pid);
        } else {
            perror("Failed to send SIGKILL");
        }
    }
}

void handleRedirection(cmdLine * pCmdLine)
{
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

int main(int argc, char **argv)
{
    char cwd[PATH_MAX];
    char input[MAX_INPUT];
    while(1){
        if (getcwd(cwd, sizeof(cwd)) != NULL) 
        {
            printf("Current working directory: %s\n", cwd);
        } 
        else 
        {
            perror("getcwd() error");
        }
        printf("Enter input here:\n");
        if (fgets(input, sizeof(input), stdin) != NULL) 
        {
            printf("You entered: %s", input);
        }
        else 
        {
            perror("fgets() error");
            return 1;
        }
        if(strcmp(input , "quit"))
        {
            break;
        }

        cmdLine* command = parseCmdLines(input); //parses the input into a cmdLine structure
        execute(command); //fork a new process and execute the command
        freeCmdLines(command); //eleases the resources allocated by parseCmdLines
    }
    return 0;
}



// // Execution Ending After execv:

// // When you use execv, it's like saying, "Stop this program and start a new one." So,
//  once execv is called and successful, your current program stops, and the new one starts. That's why the execution ends after execv.
// // Using Full Path for Executables:

// // Imagine you're looking for a book in a library. If you know exactly where the book is, 
// you go directly to that shelf and pick it up. That's what happens when you provide the full 
// path to an executable like "/bin/ls" – the system knows exactly where to find it.
// // But if you just say "Give me the book named 'ls'," the system has to search through all
//  the shelves (directories) until it finds it. That's what happens when you just use "ls" – 
//  the system searches through the directories listed in the PATH environment variable to find the executable.
// // Using execvp:

// // Why does execvp doesnt work with wildcards like '*'?

// // execvp is like saying, "Find this book for me." It looks for the executable in the directories
//  listed in the PATH environment variable.
// // But execvp can't handle special commands like "ls *," which involves using wildcards. 
// It expects a specific command without any wildcard characters because it doesn't know how to deal with them.

//Because execvp doesn't involve shell expansion, wildcards like * are treated as literal characters and are not expanded.
//Therefore, if you pass "ls *" to execvp, it will look for a file named * in the current directory, 
//rather than expanding it to match all files.


// fork: Creates a new process by duplicating the current process.
// Returns the process ID (PID) of the child process to the parent and 0 to the child.
// Returns -1 if the creation fails.

// exec: Replaces the current process with a new program.
// exec variants (like execv, execvp) are used to execute programs with different argument formats.
// Does not return if successful; returns -1 if it fails.

// signal: Sets a signal handler for a specific signal.
// Signals are software interrupts used to notify a process of events.
// Common signals include SIGINT (Ctrl+C) and SIGTERM (termination request).

// waitpid: Parent process waits for a specific child process to terminate.
// Returns the exit status of the terminated child process.
// Allows the parent process to synchronize its execution with its child processes.

// These system calls are crucial for process management, signal handling, and inter-process communication in Unix-like operating systems.