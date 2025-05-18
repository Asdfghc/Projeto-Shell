#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "../include/parser.h"
#include "../include/builtins.h"

int main() {

    //HelloWorld("printf");

    while (1) {
        char *input = NULL;
        size_t len = 0;
        
        printf("> ");
        if (getline(&input, &len, stdin) == -1) {
            perror("getline");
            free(input);
            exit(EXIT_FAILURE);
            //FIXME: ERRO
        }
        
        input[strcspn(input, "\n")] = 0;  // Remove o newline
        
        ParsedLine parsed = parse(input);
        
        Command command = parsed.commands[0].pipeline[0];  // TODO: Apenas o primeiro comando

        if (strcmp(command.argv[0], "exit") == 0) {
            free(input);
            exit(0);
        }

        // Comando interno
        if (is_builtin(command.argv[0])) {
            builtins(command);
        // Comando externo
        } else if (strncmp(command.argv[0], "./", 2) == 0) {
            // Executa o comando como um executável local
            pid_t pid = fork();
            if (pid == -1) {
                // FIXME: ERRO
                perror("fork");
                exit(EXIT_FAILURE);
            } else if (pid == 0) {  // Filho
                // Redireciona a saída padrão
                if (command.output_file != NULL) {
                    int flags = O_WRONLY | O_CREAT | (command.append ? O_APPEND : O_TRUNC);
                    int fd_out = open(command.output_file, flags);
                    dup2(fd_out, STDOUT_FILENO);
                    close(fd_out);
                }

                execvp(command.argv[0], command.argv);
                // FIXME: ERRO
                perror("execvp");
                exit(EXIT_FAILURE);
            } else {  // Pai
                // FIXME: STATUS
                int status;
                waitpid(pid, &status, 0);
            }
        // Comando não existe
        } else {
            printf("Comando não reconhecido: %s\n", command.argv[0]);  //FIXME: ERRO
        }
        
        free(input);
    }
}