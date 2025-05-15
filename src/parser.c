#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include "../include/parser.h"

ParsedLine parse(char *linha) {
    ParsedLine parsed = {0};

    // 1. Separar por &
    char *parallel_cmds[MAX_PARALLEL];
    int n_parallel = 0;
    char *token = strtok(linha, "&");
    while (token && n_parallel < MAX_PARALLEL) {
        while (*token == ' ') token++; // tira espaços à esquerda
        parallel_cmds[n_parallel++] = token;
        token = strtok(NULL, "&");
    }

    parsed.num_commands = n_parallel;

    for (int i = 0; i < n_parallel; i++) {
        ParallelCommand *pcmd = &parsed.commands[i];

        // 2. Separar por |
        char *pipe_cmds[MAX_PIPE_CMDS];
        int n_pipes = 0;
        token = strtok(parallel_cmds[i], "|");
        while (token && n_pipes < MAX_PIPE_CMDS) {
            while (*token == ' ') token++;
            pipe_cmds[n_pipes++] = token;
            token = strtok(NULL, "|");
        }

        pcmd->num_in_pipeline = n_pipes;

        for (int j = 0; j < n_pipes; j++) {
            Command *cmd = &pcmd->pipeline[j];
            char *argv[MAX_ARGS] = {0};
            int argc = 0;

            // 3. Parse do comando com redirecionamento
            token = strtok(pipe_cmds[j], " \t\n");
            while (token && argc < MAX_ARGS - 1) {
                if (strcmp(token, "<") == 0) {
                    token = strtok(NULL, " \t\n");
                    cmd->input_file = token;
                } else if (strcmp(token, ">") == 0) {
                    token = strtok(NULL, " \t\n");
                    cmd->output_file = token;
                    cmd->append = 0;
                } else if (strcmp(token, ">>") == 0) {
                    token = strtok(NULL, " \t\n");
                    cmd->output_file = token;
                    cmd->append = 1;
                } else {
                    argv[argc++] = token;
                }
                token = strtok(NULL, " \t\n");
            }
            argv[argc] = NULL;

            // Copiar argv para a struct
            for (int k = 0; k < argc; k++) {
                cmd->argv[k] = argv[k];
            }
            cmd->argv[argc] = NULL;
        }
    }

    return parsed;
}