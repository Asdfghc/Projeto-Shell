// Arquivo principal para rodar o shell
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "../include/parser.h"
#include "../include/utils.h"
/** Executa um uníco comando, tratando comandos externos, pipes e redirecionamentos
 *  command é o comando a ser executado
 *  input_pipe_fd é o descritor de pipe de entrada.
 *  output_pipe_fd é o dxescritor de pipe de saída.
 *  pipefd é array de todos os pipes do pipeline
 *  num_pipes é número de pipes no pipeline
 *  retorna o PID do processo filho criado, ou -1 caso não crie um processo (para comandos como: cd, path)
 */ 
pid_t execute_command(Command command, int input_pipe_fd, int output_pipe_fd, int **pipefd, int num_pipes, int cmd_index) {
    if (is_builtin(command.argv[0])) { // Comando interno
        if (strcmp(command.argv[0], "cd") == 0) { // Caso separado pois não cria um processo filho
            if (command.argv[1] == NULL) {
                fprintf(stderr, "cd: caminho não fornecido\n"); // ERRO
            } else {
                if (chdir(command.argv[1]) != 0) { // ERRO
                    perror("cd");
                }
            }
            return -1;
        } else if (strcmp(command.argv[0], "path") == 0) { // Caso separado pois não cria um processo filho
            if (command.argv[1] == NULL) {
                // printf("path: %s\n", getenv("PATH") ); // Print do PATH atual para depuração
                fprintf(stderr, "path: caminho não fornecido\n"); // ERRO
            } else {
                const char *old_path = getenv("PATH");
                if (old_path == NULL) old_path = "";

                char *new_path = malloc(strlen(old_path) + strlen(command.argv[1]) + 2);
                sprintf(new_path, "%s:%s", old_path, command.argv[1]);
                setenv("PATH", new_path, 1); // 1 = sobrescreve
                free(new_path);
            }
            return -1;
        } else {
            pid_t pid = fork();
            if (pid == -1) { // ERRO
                perror("fork");
                exit(EXIT_FAILURE);
            } else if (pid == 0) { // Filho
                // Fecha os pipes que não são necessários
                for (int i = 0; i < num_pipes; i++) {
                    if (pipefd[i][0] != input_pipe_fd) close(pipefd[i][0]);
                    if (pipefd[i][1] != output_pipe_fd) close(pipefd[i][1]);
                }

                // Redireciona apropriadamente a entrada e saída padrão
                redirect_io(command, input_pipe_fd, output_pipe_fd);

                command.argv[0] = prepend(command.argv[0], "/bin/"); // "/bin/<comando>"
                execvp(command.argv[0], command.argv);
                
                // ERRO
                perror("execvp");
                exit(EXIT_FAILURE);
            } else { // Pai
                return pid;
            }
        }        
    } else { // Comando externo
        // Executa o comando como um executável local
        pid_t pid = fork();
        if (pid == -1) { // ERRO
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) { // Filho
            // Fecha os pipes que não são necessários
            for (int i = 0; i < num_pipes; i++) {
                if (pipefd[i][0] != input_pipe_fd && pipefd[i][0] != -1) close(pipefd[i][0]);
                if (pipefd[i][1] != output_pipe_fd && pipefd[i][1] != -1) close(pipefd[i][1]);
            }
            
            // Redireciona apropriadamente a entrada e saída padrão
            redirect_io(command, input_pipe_fd, output_pipe_fd);

            execvp(command.argv[0], command.argv);
            
            // ERRO
            if (errno == ENOENT) {
                fprintf(stderr, "Comando '%s' não encontrado\n", command.argv[0]);
            } else {
                perror("execvp");
            }
            exit(EXIT_FAILURE);
        } else { // Pai
            return pid;
        }
    }
}

/** Função principal do Shell, tem um laço que lê e executa os comandos
 *  Retorna 0 caso de sucesso
 */
int main() {
    setenv("PATH", "", 1);
    while (1) {
        char *input = NULL;
        size_t len = 0;
        
        // Leitura da linha de comando
        printf("> ");
        if (getline(&input, &len, stdin) == -1) { // ERRO
            perror("getline");
            free(input);
            exit(EXIT_FAILURE);
        }
        input[strcspn(input, "\n")] = 0; // Remove o newline
        
        ParsedLine parsed = parse(input);

        if (parsed.num_commands == 0) { // Linha vazia ou inválida
            free(input);
            continue;
        }

        Command command = parsed.commands[0].pipeline[0];
        if (strcmp(command.argv[0], "exit") == 0) {
            free(input);
            exit(0);
        }
        
        pid_t **pids = malloc(parsed.num_commands * sizeof(pid_t *));
        if (pids == NULL) { // ERRO
            perror("malloc");
            free(input);
            exit(EXIT_FAILURE);
        }

        int *pipeline_sizes = malloc(parsed.num_commands * sizeof(int));
        if (pipeline_sizes == NULL) { // ERRO
            perror("malloc");
            free(pids);
            free(input);
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < parsed.num_commands; i++) {
            ParallelCommand pcmd = parsed.commands[i];
            int num_pipeline = pcmd.num_pipeline;
            pipeline_sizes[i] = num_pipeline;
            pids[i] = malloc(num_pipeline * sizeof(pid_t));
            int **pipefd = (int **)malloc((num_pipeline - 1) * sizeof(int *));
            if (pipefd == NULL) { // ERRO
                perror("malloc");
                free(input);
                exit(EXIT_FAILURE);
            }
            // Cria os pipes
            for (int i = 0; i < num_pipeline - 1; i++) {
                pipefd[i] = (int *)malloc(2 * sizeof(int));
                if (pipefd[i] == NULL) { // ERRO
                    perror("malloc");
                    for (int j = 0; j < i; j++) free(pipefd[j]);
                    free(pipefd);
                    free(input);
                    exit(EXIT_FAILURE);
                }
                if (pipe(pipefd[i]) == -1) { // ERRO
                    perror("pipe");
                    for (int j = 0; j <= i; j++) free(pipefd[j]);
                    free(pipefd);
                    free(input);
                    exit(EXIT_FAILURE);
                }
            }

            /* // Prints para depuração
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

            // Executa os comandos em paralelo
            for (int j = 0; j < num_pipeline; j++) {
                printf("Executando comando \"%s\"...\n", pcmd.pipeline[j].argv[0]);
                //printf("Input pipe fd: %d\n", (j == 0) ? -1 : pipefd[j - 1][0]);
                //printf("Output pipe fd: %d\n", (j == num_pipeline - 1) ? -1 : pipefd[j][1]);
                pids[i][j] = execute_command(pcmd.pipeline[j],
                    (j == 0) ? -1 : pipefd[j - 1][0],
                    (j == num_pipeline - 1) ? -1 : pipefd[j][1],
                    pipefd, num_pipeline - 1, j);
            }
            // Fecha todos os pipes no pai
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
                if (pids[i][j] == -1) { // Comando não criou um filho (cd e path)
                    continue;
                }
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
