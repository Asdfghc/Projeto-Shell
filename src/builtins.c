#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#include "../include/parser.h"
#include "../include/utils.h"

char* prepend(const char* s1, const char* s2) {
    size_t len1 = strlen(s1);
    size_t len2 = strlen(s2);
    char* result = malloc(len1 + len2 + 1); 

    if (result == NULL) {
        // FIXME: ERRO
        fprintf(stderr, "Failed to allocate memory\n");
        return NULL;
    }
    strcpy(result, s2);
    strcat(result, s1);
    return result;
}

// Verifica se o comando é um comando interno
int is_builtin(const char* command) {
    return strcmp(command, "pwd") == 0 ||
        strcmp(command, "cd") == 0 ||
        strcmp(command, "ls") == 0 ||
        strcmp(command, "cat") == 0;
        // TODO: strcmp(command.argv[0], "path") == 0
}

pid_t builtins(Command command, int input_pipe_fd, int output_pipe_fd, int **pipefd, int num_pipes) {
    // O comando "cd" não cria um processo filho
    if (strcmp(command.argv[0], "cd") == 0) {
        if (command.argv[1] == NULL) {
            fprintf(stderr, "cd: caminho não fornecido\n");  // FIXME: ERRO
        } else {
            if (chdir(command.argv[1]) != 0) {
                // FIXME: ERRO
                perror("cd");
            }
        }
        return -1; // FIXME: Deve dar errado
    }

    pid_t pid = fork();
    if (pid == -1) {
        // FIXME: ERRO
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {  // Filho
        for (int i = 0; i < num_pipes; i++) {
            if (pipefd[i][0] != input_pipe_fd) close(pipefd[i][0]);
            if (pipefd[i][1] != output_pipe_fd) close(pipefd[i][1]);
        }

        // Redireciona apropriadamente a entrada e saída padrão
        redirect_io(command, input_pipe_fd, output_pipe_fd);

        command.argv[0] = prepend(command.argv[0], "/bin/");  // "/bin/<comando>"
        execvp(command.argv[0], command.argv);
        // FIXME: ERRO
        perror("execvp");
        exit(EXIT_FAILURE);
    } else {  // Pai
        return pid;
    }
}
