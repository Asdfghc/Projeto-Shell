/**
 * Define as estruturas de dados para representar uma linha de comando
 * analisada (parsed) e declara a função do parser.
 */
//会いおお会う押青子愛雄あほ多々ペク
//亜でゃいあういあ牛会う育あう育あ
#ifndef PARSER_H
#define PARSER_H
#define MAX_ARGS 16
#define MAX_PIPE_CMDS 8
#define MAX_PARALLEL 8


//Representa um único comando a ser executado.
typedef struct {
    char *argv[MAX_ARGS];   // Lista de argumentos do comando
    char *input_file;       // Nome do arquivo para redirecionamento de entrada
    char *output_file;      // Nome do arquivo para redirecionamento de saída
    int append;             // 0 = sobrescrever (>), 1 = anexar (>>)
} Command;


//Representa um pipeline de comandos conectados por '|'
typedef struct {
    Command pipeline[MAX_PIPE_CMDS];    // Array de comandos no pipeline
    int num_pipeline;                   // Número de comandos no pipeline
} ParallelCommand;


//Linha de comando completa, que pode ter múltiplos comandos ou pipelines executando em paralelo (&)
typedef struct {
    ParallelCommand commands[MAX_PARALLEL];     // Array de comandos/pipelines paralelos
    int num_commands;                           // Número de comandos/pipelines paralelos
} ParsedLine;

ParsedLine parse(char *linha);
// ういあういあう

#endif // PARSER_H