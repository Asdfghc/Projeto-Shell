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
    if(!err) {
        fprintf(stderr, "Erro ao alocar memória!\n");
        exit(EXIT_FAILURE);
    }

    err->next = NULL;

    if(code == 0) {
        err->code = ERR_INTERNAL_ERROR_CREATION;
        err->msg = strdup("Um erro aconteceu, mas a identificação foi perdida!");
        err->file = strdup("-");
        err->line = -1;
    } else if(msg == NULL || msg[0] == '\0') {
        err->code = ERR_INTERNAL_ERROR_CREATION;
        err->msg = strdup("Um erro aconteceu, mas a informação foi perdida!");
        err->file = strdup("-");
        err->line = ERR_INTERNAL_ERROR_CREATION;
    } else if(file == NULL || file[0] == '\0') {
        err->code = ERR_INTERNAL_ERROR_CREATION;
        err->msg = strdup("Um erro aconteceu, mas a referência ao arquivo foi perdida!");
        err->file = strdup("-");
        err->line = -1;
    } else if(line < 0) {
        err->code = ERR_INTERNAL_ERROR_CREATION;
        err->msg = strdup("Um erro aconteceu, mas a referência à linha de código foi perdida!");
        err->file = strdup("-");
        err->line = -1;
    } else {
        err->code = code;
        err->msg = strdup(msg);
        err->file = strdup(file);
        err->line = line;
    }

    if(!err->msg || !err->file) {
        fprintf(stderr, "Erro ao copiar a string!\n");
        exit(EXIT_FAILURE);
    }

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
        return create_error(ERR_INTERNAL_ERROR_PASSING, "Um erro ocorreu, houve ocorreu um problema ao passá-lo!");
    }

    if(msg == NULL || msg[0] == '\0') {
        return create_error(ERR_INTERNAL_ERROR_PASSING, "Um erro ocorreu, houve ocorreu um problema ao passar sua informação!");
    }

    if(file == NULL || file[0] == '\0') {
        return create_error(ERR_INTERNAL_ERROR_PASSING, "Um erro ocorreu, houve ocorreu um problema ao passar informação sobre seu arquivo!");
    }

    if(line < 0) {
        return create_error(ERR_INTERNAL_ERROR_PASSING, "Um erro ocorreu, houve ocorreu um problema ao passar informação sobre sua linha de código!");
    }

    while(prev->next) {
        prev = prev->next;
    }
    
    Error* err = (Error*)malloc(sizeof(Error));
    if(!err) {
        fprintf(stderr, "Erro ao alocar memória!\n");
        exit(EXIT_FAILURE);
    }

    err->code = prev->code;
    err->msg = strdup(msg);
    err->file = strdup(file);
    err->line = line;
    err->next = NULL;
    prev->next = err;

    if(!err->msg || !err->file) {
        fprintf(stderr, "Erro ao copiar a string!\n");
        exit(EXIT_FAILURE);
    }

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
            "Parece que um erro aconteceu, mas foi algo interno. Por favor entre em contato caso aconteça novamente.\n"
            "Nosso contato: %s\n", CONTACT_EMAIL);
        internal = true;
    }

    if(verbose) {
        printf("%d : %s | Erro %d: %s\n", err->line, err->file, err->code, err->msg);
        err = err->next;
        while(err) {
            printf("\t%d : %s | -> | %d: %s\n", err->line, err->file, err->code, err->msg);
            err = err->next;
        }
    } else {
        printf("Erro %d: %s\n", err->code, err->msg);
    }

    if(internal) {
        printf("\033[0m");
    }
}

Error* feed_error(Error* err, bool verbose) {
    if(err->code > 100) {
        exit_with_error(err, true);
    }
    print_error(err, verbose);
    return free_error(err);
}

void exit_with_error(Error* err, bool verbose) {
    Error* aux;
    print_error(err, verbose);
    aux = free_error(err);
    exit(EXIT_FAILURE);
}

char* format_error_msg(char* prefix) {
    char* err_msg = strerror(errno);
    if(!err_msg) err_msg = "Erro desconhecido";
    size_t len = strlen(prefix) + strlen(err_msg) + 3; // +3 para "\n\t" e '\0'
    char* full_msg = malloc(len);
    if(!full_msg) {
        fprintf(stderr, "Erro ao alocar memória!\n");
        exit(EXIT_FAILURE);
    }

    snprintf(full_msg, len, "%s\n\t%s", prefix, err_msg);
    return full_msg;
}