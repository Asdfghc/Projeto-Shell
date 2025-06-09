#ifndef ERRORS_H
#define ERRORS_H

#if defined(__GNUC__) || defined(__clang__)
    #define WARN_UNUSED_RESULT __attribute__((warn_unused_result))
    #define NONNULL(...) __attribute__((nonnull(__VA_ARGS__)))
#else
    #define WARN_UNUSED_RESULT
    #define NONNULL
#endif

#define ERR_INTERNAL_ERROR_CREATION -1 // falha na criação do erro
#define ERR_INTERNAL_ERROR_PASSING -2 // falha na passagem do erro

#define ERR_MISSING_PATH_ARG 1 // ausência do argumento de path
#define ERR_INVALID_PATH 2 // path inválido
#define ERR_INVALID_COMMAND 3 // comando inválido
#define ERR_COMMAND_NOT_FOUND 4 // comando não encontrado
#define ERR_INVALID_STRING 11 // string inválida
#define ERR_FILE_OPEN_FAIL 21 // erro ao abrir arquivo
#define ERR_TOO_MANY_PARALLEL_COMMANDS 31 // excesso de comandos em paralelo
#define ERR_TOO_MANY_PIPE_COMMANDS 32 // excesso de comandos em pipeline
#define ERR_TOO_MANY_ARGUMENTS 33 // excesso de argumentos

#define ERR_ALLOC_FAIL 101 // falha ao alocar memória
#define ERR_FORK_FAIL 102 // falha ao criar novo processo
#define ERR_WAITPID_FAIL 103 // falha ao encerrar processo filho
#define ERR_IO_REDIRECTION_FAIL 104 // falha ao redirecionar I/O ou duplicar descritor
#define ERR_EOF_DETECTED 110 // falha na leitura do comando, End Of File recebido
#define ERR_READ_COMMAND_FAIL 111 // outras falhas na leitura do comando


#include <stdbool.h>

struct Error {
    int code;
    char* msg;
    char* file;
    int line;
    struct Error* next;
};
typedef struct Error Error;

struct ReturnWithError {
    void* data;
    Error* err;
};
typedef struct ReturnWithError ReturnWithError;

Error* create_error_impl(int code, char* msg, const char* file, int line);
Error* pass_error_impl(char* msg, Error* prev, const char* file, int line);

Error* create_error(int code, char* msg) WARN_UNUSED_RESULT NONNULL(2);
Error* pass_error(char* msg, Error* prev) WARN_UNUSED_RESULT NONNULL(1, 2);
Error* free_error(Error* err) WARN_UNUSED_RESULT;
void print_error(Error* err, bool verbose) NONNULL(1);
Error* feed_error(Error* err, bool verbose) WARN_UNUSED_RESULT NONNULL(1);
void exit_with_error(Error* err, bool verbose) NONNULL(1);
char* format_error_msg(char* prefix) NONNULL(1);

#define create_error(code, msg) create_error_impl((code), (msg), __FILE__, __LINE__)
#define pass_error(msg, prev) pass_error_impl((msg), (prev), __FILE__, __LINE__)

#endif

// TODO: tentar mesclar com static inline e esconder os protótipos das funções de impl