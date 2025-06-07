//Declaração de funções utilitárias para o shell
#ifndef UTILS_H
#define UTILS_H

//Concatena duas strings, colocando a s2 antes da s1
char* prepend(const char* s1, const char* s2);

//Verifica se um comando é um dos comandos internos (built-in).
int is_builtin(const char* command);

//Redireciona a entrada e saída de um processo, seja para arquivos ou para os pipes de um pipeline.
void redirect_io(Command command, int input_pipe_fd, int output_pipe_fd);

#endif // UTILS_H