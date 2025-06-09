//Implementação de funções utilitárias para o shell
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../include/parser.h"
#include "../include/errors.h"

/**Concatena duas strings, colocando a s2 antes da s1
 * s1 é a string principal
 * s2 é a string a ser adicionada
 * Retorna as duas strings concatenadas
*/
ReturnWithError prepend(const char* s1, const char* s2) {
    Error* err;
    size_t len1;
    size_t len2;
    char* result;

    if(!s1 || !s2) {
        err = create_error(ERR_INVALID_STRING, "String inválida!");
        
        return (ReturnWithError){
            NULL,
            err
        };
    }

    len1 = strlen(s1);
    len2 = strlen(s2);
    result = malloc(len1 + len2 + 1);

    if (result == NULL) {
        err = create_error(ERR_ALLOC_FAIL, "Falha ao alocar memória");
        
        return (ReturnWithError){
            NULL,
            err
        };
    }
    strcpy(result, s2);
    strcat(result, s1);

    return (ReturnWithError) {
        result,
        NULL
    };
}


/**Verifica se o comando digitado pelo usuário é um dos internos
 * command é o comando a ser verificado
 * Retorna 1 se for um comando interno e 0 caso o contrário
*/
int is_builtin(const char* command) {
    return strcmp(command, "pwd") == 0 ||
        strcmp(command, "cd") == 0 ||
        strcmp(command, "ls") == 0 ||
        strcmp(command, "cat") == 0 ||
        strcmp(command, "path") == 0 ||
        strcmp(command, "echo") == 0;
}


/**Redireciona a entrada e saída de um processo
 * command sendo a estrutura do comando
 * input_pipe_fd é o descritor de arquivo para ler a entrada de um pipe
 * output_pipe_fd é o descritor de arquivo para ler a saída de um pipe
 */
Error* redirect_io(Command command, int input_pipe_fd, int output_pipe_fd) {
    char* msg;
    Error* err;
    // Redirecionamento de entrada
    if (input_pipe_fd != -1) {  // Pipe de entrada
        if (dup2(input_pipe_fd, STDIN_FILENO) == -1) {
            msg = format_error_msg("Falha ao duplicar descritor de arquivo");
            err = create_error(ERR_IO_REDIRECTION_FAIL, msg);
            free(msg);
            return err;
        }
        close(input_pipe_fd);
    } else if (command.input_file != NULL && strlen(command.input_file) > 0) {  // Arquivo de entrada
        int fd_in = open(command.input_file, O_RDONLY, 0644);
        if (fd_in == -1) {
            msg = format_error_msg("Falha ao abrir arquivo");
            err = create_error(ERR_FILE_OPEN_FAIL, msg);
            free(msg);
            return err;
        }
        if (dup2(fd_in, STDIN_FILENO) == -1) {
            msg = format_error_msg("Falha ao duplicar descritor de arquivo");
            err = create_error(ERR_IO_REDIRECTION_FAIL, msg);
            free(msg);
            return err;
        }
        close(fd_in);
    }

    // Redirecionamento de saída
    if (output_pipe_fd != -1) {  // Pipe de saída
        if (dup2(output_pipe_fd, STDOUT_FILENO) == -1) {
            msg = format_error_msg("Falha ao duplicar descritor de arquivo");
            err = create_error(ERR_IO_REDIRECTION_FAIL, msg);
            free(msg);
            return err;
        }
        close(output_pipe_fd);
    } else if (command.output_file != NULL && strlen(command.output_file) > 0) {
        int flags = O_WRONLY | O_CREAT | (command.append ? O_APPEND : O_TRUNC);
        int fd_out = open(command.output_file, flags, 0644);
        if (fd_out == -1) {
            msg = format_error_msg("Falha ao abrir arquivo");
            err = create_error(ERR_FILE_OPEN_FAIL, msg);
            free(msg);
            return err;
        }
        if (dup2(fd_out, STDOUT_FILENO) == -1) {
            msg = format_error_msg("Falha ao duplicar descritor de arquivo");
            err = create_error(ERR_IO_REDIRECTION_FAIL, msg);
            free(msg);
            return err;
        }
        close(fd_out);
    }
    return NULL;
}
