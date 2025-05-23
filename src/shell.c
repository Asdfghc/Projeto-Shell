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
#include "../include/utils.h"

pid_t execute_command(Command command, int input_pipe_fd, int output_pipe_fd, int **pipefd, int num_pipes, int cmd_index) {
    // Comando interno
    if (is_builtin(command.argv[0])) {
        return builtins(command, input_pipe_fd, output_pipe_fd, pipefd, num_pipes);
        // Comando externo
    } else {
        // Executa o comando como um executável local
        pid_t pid = fork();
        if (pid == -1) {
            // FIXME: ERRO
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {  // Filho
            for (int i = 0; i < num_pipes; i++) {
                if (pipefd[i][0] != input_pipe_fd && pipefd[i][0] != -1) close(pipefd[i][0]);
                if (pipefd[i][1] != output_pipe_fd && pipefd[i][1] != -1) close(pipefd[i][1]);
            } // TODO: Por index por causa de pipe reutilizado??
            
            // Redireciona apropriadamente a entrada e saída padrão
            redirect_io(command, input_pipe_fd, output_pipe_fd);

            execvp(command.argv[0], command.argv);
            // FIXME: ERRO
            perror("execvp");
            exit(EXIT_FAILURE);
        } else {  // Pai
            return pid;
        }
    }
}

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

        if (parsed.num_commands == 0) {
            free(input);
            continue;  // Linha vazia ou inválida
        }

        Command command = parsed.commands[0].pipeline[0];  // TODO: Apenas o primeiro comando?
        if (strcmp(command.argv[0], "exit") == 0) {
            free(input);
            exit(0);
        }
        
        pid_t **pids = (pid_t **)malloc(parsed.num_commands * sizeof(pid_t *));  // FIXME: ERRO
        int *pipeline_sizes = malloc(parsed.num_commands * sizeof(int)); // FIXME: ERRO

        for (int i = 0; i < parsed.num_commands; i++) {
            ParallelCommand pcmd = parsed.commands[i];
            int num_pipeline = pcmd.num_pipeline;
            pipeline_sizes[i] = num_pipeline;
            pids[i] = malloc(num_pipeline * sizeof(pid_t));
            int **pipefd = (int **)malloc((num_pipeline - 1) * sizeof(int *));
            if (pipefd == NULL) {  // FIXME: ERRO
                perror("malloc");
                free(input);
                exit(EXIT_FAILURE);
            }
            for (int i = 0; i < num_pipeline - 1; i++) {
                pipefd[i] = (int *)malloc(2 * sizeof(int));
                if (pipefd[i] == NULL) {  // FIXME: ERRO
                    perror("malloc");
                    for (int j = 0; j < i; j++) free(pipefd[j]);
                    free(pipefd);
                    free(input);
                    exit(EXIT_FAILURE);
                }
                if (pipe(pipefd[i]) == -1) {  // FIXME: ERRO
                    perror("pipe");
                    for (int j = 0; j <= i; j++) free(pipefd[j]);
                    free(pipefd);
                    free(input);
                    exit(EXIT_FAILURE);
                }
            }

            /* // Print para depuração
            printf("Pipes:\n");
            for (int j = 0; j < num_pipeline - 1; j++) {
                if (pipefd[j] != NULL) {
                    printf("Pipe %d: %d -> %d\n", j, pipefd[j][1], pipefd[j][0]);
                }
            }
            for (int i = 0; i < num_pipeline; i++) {
                printf("Comando %d: ", i);
                for (int j = 0; pcmd.pipeline[i].argv[j] != NULL; j++) {
                    printf("%s ", pcmd.pipeline[i].argv[j]);
                }
                printf("\n");
                printf("Input pipe fd: %d\n", (i == 0) ? -1 : pipefd[i - 1][0]);
                printf("Output pipe fd: %d\n", (i == num_pipeline - 1) ? -1 : pipefd[i][1]);
                printf("Input file: %s\n", pcmd.pipeline[i].input_file ? pcmd.pipeline[i].input_file : "NULL");
                printf("Output file: %s\n", pcmd.pipeline[i].output_file ? pcmd.pipeline[i].output_file : "NULL");
            } */


            for (int j = 0; j < num_pipeline; j++) {
                printf("Executando comando %s\n", pcmd.pipeline[j].argv[0]);
                //printf("Input pipe fd: %d\n", (j == 0) ? -1 : pipefd[j - 1][0]);
                //printf("Output pipe fd: %d\n", (j == num_pipeline - 1) ? -1 : pipefd[j][1]);
                pids[i][j] = execute_command(pcmd.pipeline[j],
                    (j == 0) ? -1 : pipefd[j - 1][0],
                    (j == num_pipeline - 1) ? -1 : pipefd[j][1],
                    pipefd, num_pipeline - 1, j);
            }


            for (int i = 0; i < num_pipeline - 1; i++) {
                close(pipefd[i][0]);
                close(pipefd[i][1]);
                free(pipefd[i]);
            }
            free(pipefd);

            
        }
        // Espera todos os filhos
        for (int i = 0; i < parsed.num_commands; i++) {
            for (int j = 0; j < pipeline_sizes[i]; j++) {
                //printf("Esperando pid %d\n", pids[i][j]);
                if (pids[i][j] == -1) {
                    continue;  // Comando não criou um filho (cd)
                }
                // FIXME: STATUS
                int status;
                waitpid(pids[i][j], &status, 0);
            }
            free(pids[i]);
        }
        free(pids);
        free(pipeline_sizes);
        free(input);
    }
}