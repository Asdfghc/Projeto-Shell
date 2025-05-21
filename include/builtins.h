#ifndef BUILTINS_H
#define BUILTINS_H

int is_builtin(const char* command);
pid_t builtins(Command command, int input_pipe_fd, int output_pipe_fd, int **pipefd, int num_pipes);

#endif // BUILTINS_H