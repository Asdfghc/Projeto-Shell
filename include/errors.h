#ifndef ERRORS_H
#define ERRORS_H

#if defined(__GNUC__) || defined(__clang__)
    #define WARN_UNUSED_RESULT __attribute__((warn_unused_result))
    #define NONNULL(...) __attribute__((nonnull(__VA_ARGS__)))
#else
    #define WARN_UNUSED_RESULT
    #define NONNULL
#endif

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
char* format_error_msg(char* prefix) NONNULL(1);

#define create_error(code, msg) create_error_impl((code), (msg), __FILE__, __LINE__)
#define pass_error(msg, prev) pass_error_impl((msg), (prev), __FILE__, __LINE__)

#endif

// TODO: tentar mesclar com static inline e esconder os protótipos das funções de impl