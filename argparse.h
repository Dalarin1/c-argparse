#ifndef SHIT_ARGPARSE
#define SHIT_ARGPARSE

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum ArgumentType {
    ARG_CHAR,
    ARG_UCHAR,
    ARG_INT,
    ARG_UINT,
    ARG_LONG,
    ARG_ULONG,
    ARG_LONG_LONG,
    ARG_ULONG_LONG,
    ARG_STRING,
    ARG_BOOL,
    ARG_FLOAT,
    ARG_DOUBLE,
} ArgumentType;

typedef enum NargsType {
    NARGS_NONE,      // default single arg
    NARGS_CONST,     // exactly N values
    NARGS_ZERO_PLUS, // * — zero or more
    NARGS_ONE_PLUS,  // + — one or more
    NARGS_OPTIONAL,  // ? — zero or one
} NargsType;

typedef struct Argument {
    char **flags;
    size_t flags_count;
    bool is_optional;
    ArgumentType type;
    void *dest;
    bool has_value;
    /* for nargs != none */
    int *dest_arr_count;
    NargsType nargs_type;
    int nargs_count;

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
/// @brief
/// @param parser
/// @param flags набор флагов, которые будут ассоциированы с данным аргументом
///
/// Опциональные аргументы следует передавать набором слов, разделённых пробелом, и они ОБЯЗАНЫ
/// именть parser->prefix в начале.
///
/// Позиционные аргументы следует передавать ОДНИМ словом без пробелов, и аргумент не должен
/// начинаться с parser->prefix
/// @param argtype тип данных, который будет храниться после parse_args
void add_argument(ArgumentParser *parser, const char *flags, ArgumentType argtype, void *dest) {
    /* append arg to parser */

    Argument *tmp = (Argument *)malloc((parser->args_count + 1) * sizeof(Argument));
    memcpy(tmp, parser->args, parser->args_count * sizeof(Argument));
    free(parser->args);
    parser->args = tmp;
    parser->args_count++;

    /* parse argument data */

    Argument *arg = &(parser->args[parser->args_count - 1]);
    memset(arg, 0, sizeof(Argument));
    char **arg_flags = NULL;
    size_t arg_count = 0;

    size_t f_len = strlen(flags);
    bool inside_word = false;
    size_t word_begin = 0;

    for (size_t i = 0; i <= f_len; i++) {
        char c = (i < f_len) ? flags[i] : ' ';

        if (!inside_word && (isalphanum(c) || isprefix(parser, c) || c == '_')) {
            inside_word = true;
            word_begin = i;
        }

        if (inside_word && c == ' ') {
            inside_word = false;
            size_t word_len = i - word_begin;

            char **new_addr = (char **)malloc((arg_count + 1) * sizeof(char *));
            if (arg_flags) {
                for (size_t ii = 0; ii < arg_count; ii++) {
                    new_addr[ii] = arg_flags[ii];
                }
                free(arg_flags);
            }
            arg_flags = new_addr;

            arg_flags[arg_count] = (char *)malloc(word_len + 1);
            for (size_t j = 0; j < word_len; j++) {
                if (j == 0) {
                    bool pref = isprefix(parser, flags[word_begin + j]);
                    if (!arg->is_optional && pref) {
                        arg->is_optional = true;
                    }
                    if (arg->is_optional && !pref) {
                        fprintf(stderr, "All of optional flags must start with prefix");
                        if (parser->exit_on_error) {
                            exit(-1);
                        }
                    }
                    if (!arg->is_optional && !pref && arg_count >= 1) {
                        fprintf(
                            stderr,
                            "Positional argument should be only one given in _flags_ parameter");
                        if (parser->exit_on_error) {
                            exit(-1);
                        }
                    }
                }
                arg_flags[arg_count][j] = flags[word_begin + j];
            }
            arg_flags[arg_count][word_len] = '\0';
            arg_count++;
        }
    }

    arg->flags = arg_flags;
    arg->flags_count = arg_count;
    arg->type = argtype;
    arg->dest = dest;
}

/* TODO think of better architecture, maybe do __VA_ARGS__ thing */
void add_argument_n(ArgumentParser *parser, const char *flags, ArgumentType argtype, void *dest,
                    NargsType nargs_type, int nargs_count, int *dest_count) {

    add_argument(parser, flags, argtype, dest);
    Argument *arg = &parser->args[parser->args_count - 1];

    arg->nargs_type = nargs_type;
    arg->nargs_count = nargs_count;
    arg->dest_arr_count = dest_count;
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

#endif // SHIT_ARGPARSE