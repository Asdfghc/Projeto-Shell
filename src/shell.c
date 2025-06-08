// Arquivo principal para rodar o shell
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "../include/errors.h"
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
ReturnWithError execute_command(Command command, int input_pipe_fd, int output_pipe_fd, int **pipefd, int num_pipes, int cmd_index) {
    Error* err;
    ReturnWithError ret;
    if (is_builtin(command.argv[0])) {  // Comando interno
        if (strcmp(command.argv[0], "cd") == 0) {  // Caso separado pois não cria um processo filho
            if (command.argv[1] == NULL) {
                err = create_error(1, "É necessário fornecer um caminho!");
                return (ReturnWithError) {
                    NULL,
                    err
                };
            } else {
                if (chdir(command.argv[1]) != 0) {
                    err = create_error(1, "O caminho fornecido não existe!");
                    return (ReturnWithError) {
                        NULL,
                        err
                    };
                }
            }
            return (ReturnWithError) {
                (void*)-1,
                NULL
            };
        } else if (strcmp(command.argv[0], "path") == 0) {  // Caso separado pois não cria um processo filho
            if (command.argv[1] == NULL) {
                err = create_error(1, "É necessário fornecer um caminho!");
                return (ReturnWithError) {
                    NULL,
                    err
                };
            } else {
                const char *old_path = getenv("PATH");
                if (old_path == NULL) old_path = "";

                char *new_path = malloc(strlen(old_path) + strlen(command.argv[1]) + 2);
                sprintf(new_path, "%s:%s", old_path, command.argv[1]);
                setenv("PATH", new_path, 1);  // 1 = sobrescreve
                free(new_path);
            }
            return (ReturnWithError) {
                (void*)-1,
                NULL
            };
        } else {
            pid_t pid = fork();
            if (pid == -1) {
                err = create_error(1, "Falha ao criar novo processo");
                return (ReturnWithError) {
                    NULL,
                    err
                };
            } else if (pid == 0) {  // Filho
                // Fecha os pipes que não são necessários
                for (int i = 0; i < num_pipes; i++) {
                    if (pipefd[i][0] != input_pipe_fd) close(pipefd[i][0]);
                    if (pipefd[i][1] != output_pipe_fd) close(pipefd[i][1]);
                }

                // Redireciona apropriadamente a entrada e saída padrão
                err = redirect_io(command, input_pipe_fd, output_pipe_fd);
                if(err) {
                    err = pass_error("Falha ao redirecionar I/O", err);
                    return (ReturnWithError) {
                        NULL,
                        err
                    };
                }

                ret = prepend(command.argv[0], "/bin/"); // "/bin/<comando>"
                if(ret.err) {
                    err = pass_error("Erro ao obter o comando", err);
                    return (ReturnWithError) {
                        NULL,
                        err
                    };
                }

                command.argv[0] = (char*)ret.data;
                execvp(command.argv[0], command.argv);
            } else {  // Pai
                return (ReturnWithError) {
                    (void*)(intptr_t)pid,
                    NULL
                };
            }
        }        
    } else {  // Comando externo
        // Executa o comando como um executável local
        pid_t pid = fork();
        if (pid == -1) {
            err = create_error(1, "Falha ao criar novo processo");
            return (ReturnWithError) {
                NULL,
                err
            };
        } else if (pid == 0) {  // Filho
            // Fecha os pipes que não são necessários
            for (int i = 0; i < num_pipes; i++) {
                if (pipefd[i][0] != input_pipe_fd && pipefd[i][0] != -1) close(pipefd[i][0]);
                if (pipefd[i][1] != output_pipe_fd && pipefd[i][1] != -1) close(pipefd[i][1]);
            }
            
            // Redireciona apropriadamente a entrada e saída padrão
            err = redirect_io(command, input_pipe_fd, output_pipe_fd);
            if(err) {
                err = pass_error("Falha ao redirecionar I/O", err);
                return (ReturnWithError){
                    NULL,
                    err
                };
            }

            execvp(command.argv[0], command.argv);
            err = create_error(1, "O comando não foi encontrado!");
            return (ReturnWithError) {
                NULL,
                err
            };
        } else {  // Pai
            return (ReturnWithError) {
                (void*)(intptr_t)pid,
                NULL
            };
        }
    }
}

/** Função principal do Shell, tem um laço que lê e executa os comandos
 *  Retorna 0 caso de sucesso
 */
int main() {
    bool verbose_flag = false;
    Error* err;
    ReturnWithError ret;
    while (1) {
        char *input = NULL;
        size_t len = 0;
        
        // Leitura da linha de comando
        printf("> ");
        if (getline(&input, &len, stdin) == -1) {
            if(feof(stdin)) {
                err = create_error(1, "Entrada inválida (EOF detectado)");
                err = feed_error(err, verbose_flag);
            } else {
                err = create_error(1, format_error_msg("Falha ao ler o comando"));
                err = feed_error(err, verbose_flag);
            }
            free(input);
            exit(EXIT_FAILURE);
        }
        input[strcspn(input, "\n")] = 0;  // Remove o newline
        
        ParsedLine parsed = parse(input);

        if (parsed.num_commands == 0) {  // Linha vazia ou inválida
            free(input);
            continue;
        }

        Command command = parsed.commands[0].pipeline[0];  // TODO: Apenas o primeiro comando?
        if (strcmp(command.argv[0], "exit") == 0) {
            free(input);
            exit(0);
        }
        
        pid_t **pids = (pid_t **)malloc(parsed.num_commands * sizeof(pid_t *));
        if(pids == NULL) {
            err = create_error(1, format_error_msg("Falha ao alocar memória"));
            err = feed_error(err, verbose_flag);
        }

        int *pipeline_sizes = malloc(parsed.num_commands * sizeof(int));
        if(pipeline_sizes == NULL) {
            err = create_error(1, format_error_msg("Falha ao alocar memória"));
            err = feed_error(err, verbose_flag);
        }

        for (int i = 0; i < parsed.num_commands; i++) {
            ParallelCommand pcmd = parsed.commands[i];
            int num_pipeline = pcmd.num_pipeline;
            pipeline_sizes[i] = num_pipeline;
            pids[i] = malloc(num_pipeline * sizeof(pid_t));
            int **pipefd = (int **)malloc((num_pipeline - 1) * sizeof(int *));
            if (pipefd == NULL) {
                err = create_error(1, format_error_msg("Falha ao alocar memória"));
                err = feed_error(err, verbose_flag);
            }
            // Cria os pipes
            for (int i = 0; i < num_pipeline - 1; i++) {
                pipefd[i] = (int *)malloc(2 * sizeof(int));
                if (pipefd[i] == NULL) {
                    err = create_error(1, format_error_msg("Falha ao alocar memória"));
                    err = feed_error(err, verbose_flag);
                    for (int j = 0; j < i; j++) free(pipefd[j]);
                    free(pipefd);
                    free(input);
                    exit(EXIT_FAILURE);
                }
                if (pipe(pipefd[i]) == -1) {
                    err = create_error(1, format_error_msg("Falha ao alocar memória"));
                    err = feed_error(err, verbose_flag);
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
                printf("Executando comando %s...\n", pcmd.pipeline[j].argv[0]);
                //printf("Input pipe fd: %d\n", (j == 0) ? -1 : pipefd[j - 1][0]);
                //printf("Output pipe fd: %d\n", (j == num_pipeline - 1) ? -1 : pipefd[j][1]);
                
                ret = execute_command(pcmd.pipeline[j],
                    (j == 0) ? -1 : pipefd[j - 1][0],
                    (j == num_pipeline - 1) ? -1 : pipefd[j][1],
                    pipefd, num_pipeline - 1, j);

                if(ret.err) {
                    err = feed_error(ret.err, verbose_flag);
                    free(pids);
                    free(pipeline_sizes);
                    free(input);
                    exit(EXIT_FAILURE);
                }

                pids[i][j] = (pid_t)(intptr_t)ret.data;
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
                if (pids[i][j] == -1) {  // Comando não criou um filho (cd e path)
                    continue;
                }

                int status;
                if(waitpid(pids[i][j], &status, 0) == -1) {
                    err = create_error(1, "Falha ao encerrar processo!");
                    err = feed_error(err, verbose_flag);
                }
            }
            free(pids[i]);
        }
        free(pids);
        free(pipeline_sizes);
        free(input);
    }
}