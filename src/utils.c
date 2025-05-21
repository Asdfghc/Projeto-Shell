#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include "../include/parser.h"

void redirect_io(Command command, int input_pipe_fd, int output_pipe_fd) {
    // Redirecionamento de entrada
    if (input_pipe_fd != -1) {
        if (dup2(input_pipe_fd, STDIN_FILENO) == -1) {  // FIXME: ERRO
            perror("dup2 input_pipe_fd");
            exit(EXIT_FAILURE);
        }
        //printf("input_pipe_fd: %d\n", input_pipe_fd);
        close(input_pipe_fd);
    } else if (command.input_file != NULL) {
        int fd_in = open(command.input_file, O_RDONLY);
        if (fd_in == -1) {  // FIXME: ERRO
            perror("open input_file");
            exit(EXIT_FAILURE);
        }
        if (dup2(fd_in, STDIN_FILENO) == -1) {  // FIXME: ERRO
            perror("dup2 input_file");
            exit(EXIT_FAILURE);
        }
        //printf("input_file: %s\n", command.input_file);
        close(fd_in);
    }

    // Redirecionamento de saída
    if (output_pipe_fd != -1) {
        if (dup2(output_pipe_fd, STDOUT_FILENO) == -1) {  // FIXME: ERRO
            perror("dup2 output_pipe_fd");
            exit(EXIT_FAILURE);
        }
        //printf("output_pipe_fd: %d\n", output_pipe_fd);
        close(output_pipe_fd);
    } else if (command.output_file != NULL) {
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
        //printf("output_file: %s\n", command.output_file);
        close(fd_out);
    }
}
