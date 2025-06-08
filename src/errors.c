#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "../include/environment.h"
#include "../include/errors.h"

/*
São dois tipos de erros: erros de aplicação e erros de comando.

 - Erros de aplicação são aqueles que demonstram problemas no código que implementa o shell.
   São printados em vermelho.

 - Erros de comando são aqueles que demonstram problemas na execução dos comandos do shell.
   São printados na cor padrão.

Erros podem ser analisados com mais detalhes com a flag --verbose, que mostra sua propagação (stacktrace).

Todas as funções do projeto, salvo as presentes nesta biblioteca e a main, devem ter seu retorno adequado à seguinte regra:
se um erro pode ocorrer dentro da função, seu retorno deve ser do tipo ReturnWithError. O retorno original da função deve se
ser definido no campo "data".
*/

// TODO: definir códigos de erro e classificações
// negativo significa erro provavelmente interno, enquanto positivo significa erro por ação do usuário
// TODO: forma de enviar um relatório com os comandos utilizados e especificações do sistema/ambiente para erros internos
// TODO: warn_unused_result, non null param

Error* create_error_impl(int code, char *msg, const char* file, int line) {
    Error* err = (Error*)malloc(sizeof(Error));
    err->next = NULL;

    if(code == 0) {
        err->code = -1;
        err->msg = strdup("An error occurred, but the identification was lost!");
        err->file = strdup("-");
        err->line = -1;

        return err;
    }

    if(msg == NULL || msg[0] == '\0') {
        err->code = -1;
        err->msg = strdup("An error occurred, but the information was lost!");
        err->file = strdup("-");
        err->line = -1;

        return err;
    }

    if(file == NULL || file[0] == '\0') {
        err->code = -1;
        err->msg = strdup("An error occurred, but the reference to the file was lost!");
        err->file = strdup("-");
        err->line = -1;
    }

    if(line < 0) {
        err->code = -1;
        err->msg = strdup("An error occurred, but the reference to the code line was lost!");
        err->file = strdup("-");
        err->line = -1;
    }

    err->code = code;
    err->msg = strdup(msg);
    err->file = strdup(file);
    err->line = line;
    
    return err;
}

Error* free_error(Error* err) {
    Error* aux;

    while(err) {
        aux = err->next;
        if(err->msg) free(err->msg);
        free(err);
        err = aux;
    }

    return NULL;
}

Error* pass_error_impl(char* msg, Error* prev, const char* file, int line) {
    if(!prev) {
        return create_error(-2, "An error occurred, but there was a problem while passing it!");
    }

    if(msg == NULL || msg[0] == '\0') {
        return create_error(-2, "An error occurred, but there was a problem while passing its information!");
    }

    if(file == NULL || file[0] == '\0') {
        return create_error(-2, "An error ocurred but there was a problem while passing information about its file!");
    }

    if(line < 0) {
        return create_error(-2, "An error ocurred but there was a problem while passing information about its code line!");
    }

    while(prev->next) {
        prev = prev->next;
    }
    
    Error* err = (Error*)malloc(sizeof(Error));
    err->code = prev->code;
    err->msg = strdup(msg);
    err->file = strdup(file);
    err->line = line;
    err->next = NULL;
    prev->next = err;

    return prev;
}

void print_error(Error* err, bool verbose) {
    bool internal = false;
    if(!err) {
        return;
    }

    if(err->code < 0) {
        printf(
            "\033[31m"
            "It seens an error occurred, but it was internal. Please, contact us if that happens again.\n"
            "Our contact: %s\n", CONTACT_EMAIL);
        internal = true;
    }

    if(verbose) {
        err = err->next;
        printf("%d : %s | Error %d: %s\n", err->line, err->file, err->code, err->msg);
        while(err) {
            printf("\t%d : %s | -> | %d: %s\n", err->line, err->file, err->code, err->msg);
            err = err->next;
        }
    } else {
        printf("Error %d: %s\n", err->code, err->msg);
    }

    if(internal) {
        printf("\033[0m");
    }
}

Error* feed_error(Error* err, bool verbose) {
    print_error(err, verbose);
    return free_error(err);
}

char* format_error_msg(char* prefix) {
    char* err_msg = strerror(errno);
    size_t len = strlen(prefix) + strlen(err_msg) + 3; // +3 para "\n\t" e '\0'
    char* full_msg = malloc(len);
    if (!full_msg) return NULL;

    snprintf(full_msg, len, "%s\n\t%s", prefix, err_msg);
    return full_msg;
}