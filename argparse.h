#ifndef SHIT_ARGPARSE
#define SHIT_ARGPARSE

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define add_argument(parser, flags, dest, ...)                                                     \
    __add_argument_base(parser, flags, dest, (ArgumentOptions){__VA_ARGS__})

typedef struct {
    char **items;
    size_t count;
} StringSplitResult;

void free_split_result(StringSplitResult *res) {
    if (!res) {
        return;
    }
    for (size_t i = 0; i < res->count; i++) {
        free(res->items[i]);
    }

    free(res->items);
    res->items = NULL;
    res->count = 0;
}

StringSplitResult string_split(const char *str) {
    StringSplitResult result = {0};

    size_t len = strlen(str);
    bool inside_word = false;
    size_t word_begin = 0;

    for (size_t i = 0; i <= len; i++) {
        char c = (i < len) ? str[i] : ' ';
        if (!inside_word && c != ' ') {
            inside_word = true;
            word_begin = i;
        }

        if (inside_word && c == ' ') {
            inside_word = false;
            size_t word_len = i - word_begin;
            char **tmp = (char **)realloc(result.items, (result.count + 1) * sizeof(char *));

            if (!tmp) {
                free_split_result(&result);
                return (StringSplitResult){0};
            }

            result.items = tmp;
            result.items[result.count] = (char *)malloc(word_len + 1);

            if (!result.items[result.count]) {
                free_split_result(&result);
                return (StringSplitResult){0};
            }

            memcpy(result.items[result.count], str + word_begin, word_len);
            result.items[result.count][word_len] = '\0';
            result.count++;
        }
    }

    return result;
}

typedef enum ArgumentType {
    ARG_BOOL,
    ARG_CHAR,
    ARG_UCHAR,
    ARG_INT,
    ARG_UINT,
    ARG_LONG,
    ARG_ULONG,
    ARG_LONG_LONG,
    ARG_ULONG_LONG,
    ARG_STRING,
    ARG_FLOAT,
    ARG_DOUBLE,
} ArgumentType;

typedef enum {
    ACTION_STORE,       // default
    ACTION_STORE_CONST, // --flag (сохранить константу)
    ACTION_STORE_TRUE,  // --verbose (сохранить true)
    ACTION_STORE_FALSE,
    ACTION_APPEND, // можно указать несколько раз
    ACTION_APPEND_CONST,
    ACTION_COUNT, // -vvv -> 3
    ACTION_HELP,
    ACTION_VERSION
} ArgumentAction;

typedef enum NargsType {
    NARGS_NONE,           // default single arg
    NARGS_CONST,          // exactly N values
    NARGS_ZERO_PLUS = -1, // * — zero or more
    NARGS_ONE_PLUS = -2,  // + — one or more
    NARGS_OPTIONAL = -3,  // ? — zero or one
} NargsType;

typedef struct Argument {
    char **flags;
    size_t flags_count;
    bool is_optional;
    ArgumentType type;
    void *dest;
    bool has_value;
    bool required;
    /* for nargs != none */
    int *dest_arr_count;
    NargsType nargs_type;
    int nargs_count;
    ArgumentAction action;
} Argument;

void argument_read_value(Argument *arg, char *strval, bool to_arr, int arr_idx) {
    int offset = to_arr ? arr_idx : 0;
    long long tmp = (long long)(arg->dest);
    if (to_arr) {
        arg->dest = *(void **)(arg->dest);
    }
    switch (arg->type) {
    case ARG_CHAR:
        *((char *)(arg->dest) + offset) = (char)atoi(strval);
        break;
    case ARG_UCHAR:
        *((unsigned char *)(arg->dest) + offset) = (unsigned char)atoi(strval);
        break;
    case ARG_INT:
        *((int *)(arg->dest) + offset) = atoi(strval);
        break;
    case ARG_UINT:
        *((unsigned int *)(arg->dest) + offset) = (unsigned int)strtoul(strval, NULL, 10);
        break;
    case ARG_LONG:
        *((long *)(arg->dest) + offset) = strtol(strval, NULL, 10);
        break;
    case ARG_ULONG:
        *((unsigned long *)(arg->dest) + offset) = strtoul(strval, NULL, 10);
        break;
    case ARG_LONG_LONG:
        *((long long *)(arg->dest) + offset) = strtoll(strval, NULL, 10);
        break;
    case ARG_ULONG_LONG:
        *((unsigned long long *)(arg->dest) + offset) = strtoull(strval, NULL, 10);
        break;
    case ARG_FLOAT:
        *((float *)(arg->dest) + offset) = strtof(strval, NULL);
        break;
    case ARG_DOUBLE:
        *((double *)(arg->dest) + offset) = strtod(strval, NULL);
        break;
    case ARG_STRING:
        *((char **)(arg->dest) + offset) = strdup(strval);
        break;
    }
    if (to_arr) {
        arg->dest = (void *)(tmp);
    }
}

typedef struct ArgumentParser {
    char *prog;
    char *usage;
    char *description;
    char *epilog;
    char *prefix_chars;

    size_t usage_size;
    size_t prog_size;
    size_t description_size;
    size_t epilog_size;
    size_t prefix_chars_size;

    bool add_help;
    bool allow_abbrev;
    bool exit_on_error;
    bool suggest_on_error;
    bool color;

    Argument *args;
    size_t args_count;

} ArgumentParser;

ArgumentParser *parser_new(const char *prog, const char *description) {
    ArgumentParser *parser = (ArgumentParser *)malloc(sizeof(ArgumentParser));
    if (!parser) {
        return NULL;
    }

    memset(parser, 0, sizeof(ArgumentParser));

    parser->prog = prog ? strdup(prog) : NULL;
    parser->description = description ? strdup(description) : NULL;

    parser->prefix_chars = malloc(sizeof(char));
    parser->prefix_chars[0] = '-';
    parser->prefix_chars_size = 1;

    parser->add_help = true;
    parser->allow_abbrev = true;
    parser->exit_on_error = true;
    parser->suggest_on_error = false;
    parser->color = true;

    return parser;
}

void parser_free(ArgumentParser *parser) {
    if (!parser) {
        return;
    }
    for (size_t i = 0; i < parser->args_count; i++) {
        for (size_t k = 0; k < parser->args[i].flags_count; k++) {
            free(parser->args[i].flags[k]);
        }
        free(parser->args[i].flags);
    }

    free((void *)parser->prog);
    free((void *)parser->usage);
    free((void *)parser->description);
    free((void *)parser->epilog);
    free((void *)parser->prefix_chars);
    free((void *)parser->args);

    free((void *)parser);
}

bool isalphanum(char c) {
    return ('0' <= c && c <= '9') || ('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z');
}

bool isprefix(ArgumentParser *parser, char c) {
    for (size_t i = 0; i < parser->prefix_chars_size; i++) {
        if (c == parser->prefix_chars[i]) {
            return true;
        }
    }
    return false;
}

Argument *findflag(ArgumentParser *parser, char *str) {
    for (size_t i = 0; i < parser->args_count; i++) {
        for (size_t j = 0; j < parser->args[i].flags_count; j++) {
            if (strcmp(parser->args[i].flags[j], str) == 0) {
                return &parser->args[i];
            }
        }
    }
    return NULL;
}

int get_argtype_size(ArgumentType type) {
    switch (type) {
    case ARG_CHAR:
    case ARG_UCHAR:
        return sizeof(char);
    case ARG_INT:
    case ARG_UINT:
        return sizeof(int);
    case ARG_LONG:
    case ARG_ULONG:
        return sizeof(long);
    case ARG_LONG_LONG:
    case ARG_ULONG_LONG:
        return sizeof(long long);
    case ARG_STRING:
        return sizeof(char *);
    case ARG_BOOL:
        return sizeof(bool);
    case ARG_FLOAT:
        return sizeof(float);
    case ARG_DOUBLE:
        return sizeof(double);
    default:
        break;
    }
}

void parse_args(ArgumentParser *parser, int argc, char **args) {
    for (size_t i = 0; i < argc; i++) {
        bool optional = isprefix(parser, args[i][0]);
        Argument *arg = NULL;
        if (!optional) {
            bool found = false;
            for (size_t j = 0; j < parser->args_count; j++) {
                arg = &parser->args[j];
                if (!arg->is_optional && !arg->has_value) {
                    argument_read_value(arg, args[i], false, 0);
                    arg->has_value = true;
                    found = true;
                    break;
                }
            }
            if (!found) {
                fprintf(stderr, "error: unexpected positional argument: %s\n", args[i]);
                if (parser->exit_on_error) {
                    exit(-1);
                }
            }
            continue; // не идём в цикл по флагам
        }

        arg = findflag(parser, args[i]);
        if (!arg) {
            fprintf(stderr, "Unrecongnized argument: %s\n", args[i]);
            if (parser->exit_on_error) {
                exit(-1);
            }
        }
        if (arg->has_value) {
            fprintf(stderr, "error: argument %s: expected one argument", arg->flags[0]);
            if (parser->exit_on_error) {
                exit(-1);
            }
        }

        if (arg->type == ARG_BOOL) {
            *((bool *)arg->dest) = true;
            arg->has_value = true;
            continue;
        } else {
            if (arg->nargs_type == NARGS_NONE) {
                i++;
                if (i >= argc || findflag(parser, args[i])) {
                    fprintf(stderr, "error: argument %s: expected value", arg->flags[0]);
                    if (parser->exit_on_error) {
                        exit(-1);
                    }
                } else {
                    /* try parse value from char* to arg->type */
                    argument_read_value(arg, args[i], false, 0);
                    arg->has_value = true;
                }
            } else {
                switch (arg->nargs_type) {
                case NARGS_CONST: {
                    if (!(*(void **)(arg->dest))) {
                        *((void **)(arg->dest)) =
                            malloc(get_argtype_size(arg->type) * arg->nargs_count);
                    }
                    for (size_t ii = 0; ii < arg->nargs_count; ii++) {
                        i++;
                        if (i >= argc || findflag(parser, args[i])) {
                            fprintf(stderr, "error: argument %s: expected exact %d values",
                                    arg->flags[0], arg->nargs_count);
                            if (parser->exit_on_error) {
                                exit(-1);
                            }
                        } else {
                            argument_read_value(arg, args[i], true, ii);
                        }
                    }
                    arg->has_value = true;
                    if (arg->dest_arr_count) {
                        *(arg->dest_arr_count) = arg->nargs_count;
                    }
                    break;
                }
                case NARGS_OPTIONAL: {
                    i++;
                    if (i < argc && !findflag(parser, args[i])) {
                        if (!(*(void **)(arg->dest))) {
                            *((void **)(arg->dest)) = malloc(get_argtype_size(arg->type));
                        }
                        argument_read_value(arg, args[i], false, 0);
                        arg->has_value = true;
                        *(arg->dest_arr_count) = 1;
                    } else {
                        i--;
                    }
                    break;
                }
                case NARGS_ZERO_PLUS: {
                    i++;
                    if (i >= argc || findflag(parser, args[i])) {
                        i--;
                    } else {
                        if (!(*(void **)(arg->dest))) {
                            *((void **)(arg->dest)) = malloc(get_argtype_size(arg->type));
                        }
                        arg->nargs_count = 0;
                        int offset = 0;
                        while (i < argc && !findflag(parser, args[i])) {
                            *((void **)(arg->dest)) =
                                realloc(*((void **)(arg->dest)),
                                        get_argtype_size(arg->type) * (arg->nargs_count + 1));
                            argument_read_value(arg, args[i], true, offset);
                            arg->nargs_count++;
                            offset++;
                            i++;
                        }
                        i--;
                        *(arg->dest_arr_count) = arg->nargs_count;
                    }
                    break;
                }
                case NARGS_ONE_PLUS: {
                    i++;
                    if (i >= argc || findflag(parser, args[i])) {
                        fprintf(stderr, "error: argument %s: expected at least 1 value",
                                arg->flags[0]);
                        if (parser->exit_on_error) {
                            exit(-1);
                        }
                    } else {
                        if (!(*(void **)(arg->dest))) {
                            *((void **)(arg->dest)) = malloc(get_argtype_size(arg->type));
                        }
                        int offset = 0;
                        arg->nargs_count = 0;
                        do {

                            *((void **)(arg->dest)) =
                                realloc(*((void **)(arg->dest)),
                                        get_argtype_size(arg->type) *
                                            (arg->nargs_count > 0 ? arg->nargs_count : 1));
                            argument_read_value(arg, args[i], true, offset);
                            arg->nargs_count++;
                            offset++;
                            i++;
                        } while (i < argc && !findflag(parser, args[i]));
                        i--;
                        *(arg->dest_arr_count) = arg->nargs_count;
                        break;
                    }
                    break;
                }
                default:
                    break;
                }
            }
        }
    }

    bool found = false;
    for (size_t j = 0; j < parser->args_count; j++) {
        Argument *arg = &parser->args[j];
        if (!arg->is_optional && !arg->has_value) {
            fprintf(stderr, "Positional argument \"%s\" not set", arg->flags[0]);
            if (parser->exit_on_error) {
                exit(-1);
            }
        }
    }
}

typedef struct {
    ArgumentAction action;
    ArgumentType type;
    NargsType nargs;
    bool required;
    int count;
    int *dest_count;
} ArgumentOptions;

void validate_arg_flags(ArgumentParser *parser, Argument *arg) {
    for (size_t i = 0; i < arg->flags_count; i++) {
        char *flag = arg->flags[i];

        bool pref = isprefix(parser, flag[0]);

        if (!arg->is_optional && pref) {
            arg->is_optional = true;
        }

        if (arg->is_optional && !pref) {
            fprintf(stderr, "All optional flags must start with prefix");
            if (parser->exit_on_error) {
                exit(-1);
            }
        }

        if (!arg->is_optional && !pref && i >= 1) {
            fprintf(stderr, "Positional argument should be only one "
                            "given in _flags_ parameter");

            if (parser->exit_on_error) {
                exit(-1);
            }
        }
    }
}

void __add_argument_base(ArgumentParser *parser, const char *flags, void *dest,
                         ArgumentOptions opt) {
    parser->args = (Argument *)realloc(parser->args, (parser->args_count + 1) * sizeof(Argument));
    parser->args_count++;

    Argument *arg = &(parser->args[parser->args_count - 1]);
    memset(arg, 0, sizeof(Argument));

    StringSplitResult res = string_split(flags);

    arg->flags = res.items;
    arg->flags_count = res.count;

    validate_arg_flags(parser, arg);

    arg->dest = dest;
    arg->type = opt.type;
    arg->action = opt.action;
    if (opt.required) {
        arg->required = opt.required;
        arg->is_optional = false;
    }

    arg->nargs_type = opt.nargs;
    if (opt.nargs >= 0) { // NONE OR CONST
        if (opt.count) {
            arg->nargs_type = NARGS_CONST;
            arg->nargs_count = opt.count;
        }
    }
}

#endif // SHIT_ARGPARSE