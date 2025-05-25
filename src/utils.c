#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../include/parser.h"

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

int is_builtin(const char* command) {
    return strcmp(command, "pwd") == 0 ||
        strcmp(command, "cd") == 0 ||
        strcmp(command, "ls") == 0 ||
        strcmp(command, "cat") == 0 ||
        strcmp(command, "path") == 0 ||
        strcmp(command, "echo") == 0;
}

void redirect_io(Command command, int input_pipe_fd, int output_pipe_fd) {
    // Redirecionamento de entrada
    if (input_pipe_fd != -1) {  // Pipe de entrada
        if (dup2(input_pipe_fd, STDIN_FILENO) == -1) {  // FIXME: ERRO
            perror("dup2 input_pipe_fd");
            exit(EXIT_FAILURE);
        }
        // printf("input_pipe_fd: %d\n", input_pipe_fd);
        close(input_pipe_fd);
    } else if (command.input_file != NULL) {  // Arquivo de entrada
        int fd_in = open(command.input_file, O_RDONLY, 0644);
        if (fd_in == -1) {  // FIXME: ERRO
            perror("open input_file");
            exit(EXIT_FAILURE);
        }
        if (dup2(fd_in, STDIN_FILENO) == -1) {  // FIXME: ERRO
            perror("dup2 input_file");
            exit(EXIT_FAILURE);
        }
        close(fd_in);
    }

    // Redirecionamento de saída
    if (output_pipe_fd != -1) {  // Pipe de saída
        if (dup2(output_pipe_fd, STDOUT_FILENO) == -1) {  // FIXME: ERRO
            perror("dup2 output_pipe_fd");
            exit(EXIT_FAILURE);
        }
        close(output_pipe_fd);
    } else if (command.output_file != NULL) {  // Arquivo de saída
        int flags = O_WRONLY | O_CREAT | (command.append ? O_APPEND : O_TRUNC);
        int fd_out = open(command.output_file, flags, 0644);
        if (fd_out == -1) {  // FIXME: ERRO
            perror("open output_file");
            exit(EXIT_FAILURE);
        }
        if (dup2(fd_out, STDOUT_FILENO) == -1) {  // FIXME: ERRO
            perror("dup2 output_file");
            exit(EXIT_FAILURE);
        }
        close(fd_out);
    }
}
