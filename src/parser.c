#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "../include/parser.h"

ParsedLine parse(char *linha) {
    ParsedLine parsed = {0};

    if (linha == NULL || *linha == '\0' ||
        strspn(linha, " \t\n") == strlen(linha)) {
        return parsed;
    }

    // 1. Separar por &
    char *parallel_cmds[MAX_PARALLEL];
    int n_parallel = 0;
    char *saveptr1;
    char *token1 = strtok_r(linha, "&", &saveptr1);
    while (token1 && n_parallel < MAX_PARALLEL) {
        while (*token1 == ' ') token1++;  // tira espaços à esquerda
        parallel_cmds[n_parallel++] = token1;
        token1 = strtok_r(NULL, "&", &saveptr1);
    }

    parsed.num_commands = n_parallel;

    for (int i = 0; i < n_parallel; i++) {
        ParallelCommand *pcmd = &parsed.commands[i];

        // 2. Separar por |
        char *pipe_cmds[MAX_PIPE_CMDS];
        int n_pipes = 0;
        char *saveptr2;
        char *token2 = strtok_r(parallel_cmds[i], "|", &saveptr2);
        while (token2 && n_pipes < MAX_PIPE_CMDS) {
            while (*token2 == ' ') token2++;
            pipe_cmds[n_pipes++] = token2;
            token2 = strtok_r(NULL, "|", &saveptr2);
        }

        pcmd->num_pipeline = n_pipes;

        for (int j = 0; j < n_pipes; j++) {
            Command *cmd = &pcmd->pipeline[j];
            char *argv[MAX_ARGS] = {0};
            int argc = 0;

            // 3. Parse do comando com redirecionamento
            char *saveptr3;
            char *token3 = strtok_r(pipe_cmds[j], " \t\n", &saveptr3);
            while (token3 && argc < MAX_ARGS - 1) {
                if (strcmp(token3, "<") == 0) {
                    token3 = strtok_r(NULL, " \t\n", &saveptr3);
                    cmd->input_file = token3;
                } else if (strcmp(token3, ">") == 0) {
                    token3 = strtok_r(NULL, " \t\n", &saveptr3);
                    cmd->output_file = token3;
                    cmd->append = 0;
                } else if (strcmp(token3, ">>") == 0) {
                    token3 = strtok_r(NULL, " \t\n", &saveptr3);
                    cmd->output_file = token3;
                    cmd->append = 1;
                } else {
                    argv[argc++] = token3;
                }
                token3 = strtok_r(NULL, " \t\n", &saveptr3);
            }

            argv[argc] = NULL;

            for (int k = 0; k < argc; k++) {
                cmd->argv[k] = argv[k];
            }
            cmd->argv[argc] = NULL;
        }
    }

    return parsed;
}
