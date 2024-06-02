#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h> 
#include <sys/wait.h>  

int main() {
    int pipefd[2];
    pid_t pid;
    char writeMsg[] = "hello";
    char readChar;

    // Create a pipe
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(1);
    }

    // Fork a child process
    pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(1);
    }

    if (pid == 0) {
        // Child process
        close(pipefd[0]); // Close unused read end

        // Write a message to the pipe
        if (write(pipefd[1], writeMsg, strlen(writeMsg)) == -1) {
            perror("write");
            exit(1);
        }

        close(pipefd[1]); // Close the write end after writing
        exit(EXIT_SUCCESS);
    } else {
        // Parent process
        close(pipefd[1]); // Close unused write end

        // Read and print each character from the pipe
        printf("Received message: ");
        while (read(pipefd[0], &readChar, 1) > 0) {
            putchar(readChar);
        }
        putchar('\n');

        close(pipefd[0]); // Close the read end after reading
        wait(NULL); // Wait for the child process to terminate
    }

    return 0;
}
