#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    int pipefd[2];
    pid_t child1, child2;

    // Create a pipe
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    fprintf(stderr, "(parent_process>forking...)\n");
    child1 = fork();
    if (child1 == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (child1 == 0) {
        // First child process
        fprintf(stderr, "(child1>redirecting stdout to the write end of the pipe...)\n");
        close(pipefd[0]); // Close unused read end
        dup2(pipefd[1], STDOUT_FILENO); // Redirect stdout to pipe write end
        close(pipefd[1]); // Close pipe write end
        execlp("ls", "ls", "-l", NULL); // Execute ls -l
        perror("execlp");
        exit(EXIT_FAILURE);
    } else { //Parent
        fprintf(stderr, "(parent_process>created process with id: %d)\n", child1);
        fprintf(stderr, "(parent_process>closing the write end of the pipe...)\n");
        close(pipefd[1]); // Close write end to signal end of input

        fprintf(stderr, "(parent_process>forking...)\n");
        child2 = fork();
        if (child2 == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        }

        if (child2 == 0) {
            // Second child process
            fprintf(stderr, "(child2>redirecting stdin to the read end of the pipe...)\n");
            close(pipefd[1]); // Close unused write end
            dup2(pipefd[0], STDIN_FILENO); // Redirect stdin to pipe read end
            close(pipefd[0]); // Close pipe read end
            execlp("tail", "tail", "-n", "2", NULL); // Execute tail -n 2
            perror("execlp");
            exit(EXIT_FAILURE);
        } else { //Parent
            fprintf(stderr, "(parent_process>created process with id: %d)\n", child2);
            fprintf(stderr, "(parent_process>closing the read end of the pipe...)\n");
            close(pipefd[0]); // Close read end

            fprintf(stderr, "(parent_process>waiting for child processes to terminate...)\n");
            waitpid(child1, NULL, 0); // Wait for child1 to finish
            waitpid(child2, NULL, 0); // Wait for child2 to finish

            fprintf(stderr, "(parent_process>exiting...)\n");
        }
    }
    return EXIT_SUCCESS;
}
