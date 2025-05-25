#ifndef UTILS_H
#define UTILS_H

char* prepend(const char* s1, const char* s2);
int is_builtin(const char* command);
void redirect_io(Command command, int input_pipe_fd, int output_pipe_fd);

#endif // UTILS_H