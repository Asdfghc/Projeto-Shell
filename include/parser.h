#ifndef PARSER_H
#define PARSER_H
//会いおお会う押青子愛雄あほ多々ペク
//亜でゃいあういあ牛会う育あう育あ

#define MAX_ARGS 16
#define MAX_PIPE_CMDS 8
#define MAX_PARALLEL 8

typedef struct {
    char *argv[MAX_ARGS];
    char *input_file;
    char *output_file;
    int append;
} Command;

typedef struct {
    Command pipeline[MAX_PIPE_CMDS];
    int num_in_pipeline;
} ParallelCommand;

typedef struct {
    ParallelCommand commands[MAX_PARALLEL];
    int num_commands;
} ParsedLine;

ParsedLine parse(char *linha);
// ういあういあう

#endif // PARSER_H