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
    ACTION_STORE_CONST, // --flag (no value), add .const=(value)
    ACTION_STORE_TRUE,  // --verbose (сохранить true)
    ACTION_STORE_FALSE,
    ACTION_APPEND,       // можно указать несколько раз
    ACTION_APPEND_CONST, // при использовании флага добавляет в массив(!) dest значение .const
    ACTION_COUNT,        // -vvv -> 3
    ACTION_HELP,
    ACTION_VERSION
} ArgumentAction;

typedef enum NargsType {
    NARGS_NONE = 0,       // default single arg
    NARGS_CONST = 1,      // exactly N values
    NARGS_ZERO_PLUS = -1, // * — zero or more
    NARGS_ONE_PLUS = -2,  // + — one or more
    NARGS_OPTIONAL = -3,  // ? — zero or one
} NargsType;

typedef union {
    bool b;
    char c;
    unsigned char uc;
    int i;
    unsigned int ui;
    long l;
    unsigned long ul;
    long long ll;
    unsigned long long ull;
    float f;
    double d;
    char *s;
    void *ptr;
} ArgumentValue;

typedef struct Argument {
    char **flags;
    size_t flags_count;
    bool is_optional;
    ArgumentType type;
    void *dest;
    bool positional;
    bool has_value;
    bool required;
    /* for nargs != none */
    int *dest_arr_count;
    NargsType nargs_type;
    int nargs_count;
    ArgumentAction action;
    ArgumentValue const_val;
    ArgumentValue default_val;
} Argument;

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
void *get_write_addr(Argument *arg, bool to_arr, int offset) {
    if (to_arr) {
        void *res = *(void **)arg->dest;
        res = (char *)res + offset * get_argtype_size(arg->type);
        return res;
    }
    return arg->dest;
}
// clang-format off
void argument_read_value(Argument *arg, char *strval, bool to_arr, int arr_idx) {
    void *write_addr = get_write_addr(arg, to_arr, arr_idx);

    switch (arg->type) {
    case ARG_CHAR:        *((char *)write_addr) = (char)atoi(strval);                               break;
    case ARG_UCHAR:       *((unsigned char *)write_addr) = (unsigned char)atoi(strval);             break;
    case ARG_INT:         *((int *)write_addr) = atoi(strval);                                      break;
    case ARG_UINT:        *((unsigned int *)write_addr) = (unsigned int)strtoul(strval, NULL, 10);  break;
    case ARG_LONG:        *((long *)write_addr) = strtol(strval, NULL, 10);                         break;
    case ARG_ULONG:       *((unsigned long *)write_addr) = strtoul(strval, NULL, 10);               break;
    case ARG_LONG_LONG:   *((long long *)write_addr) = strtoll(strval, NULL, 10);                   break;
    case ARG_ULONG_LONG:  *((unsigned long long *)write_addr) = strtoull(strval, NULL, 10);         break;
    case ARG_FLOAT:       *((float *)write_addr) = strtof(strval, NULL);                            break;
    case ARG_DOUBLE:      *((double *)write_addr) = strtod(strval, NULL);                           break;
    case ARG_STRING:      *((char **)write_addr) = strdup(strval);                                  break;
    }
}
// clang-format on

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

Argument *findnextpositional(ArgumentParser *parser) {
    Argument *arg = NULL;
    for (size_t j = 0; j < parser->args_count; j++) {
        arg = &parser->args[j];
        if (arg->positional && !arg->has_value) {
            return arg;
        }
    }
    return NULL;
}

// clang-format off
void set_arg_dest(Argument *arg, ArgumentValue value) {
    switch (arg->type) {
    case ARG_BOOL:       *(bool *)arg->dest = (value.b);                  break;
    case ARG_CHAR:       *(char *)arg->dest = (value.c);                  break;
    case ARG_UCHAR:      *(unsigned char *)arg->dest = (value.uc);        break;
    case ARG_INT:        *(int *)arg->dest = (value.i);                   break;
    case ARG_UINT:       *(unsigned int *)arg->dest = (value.ui);         break;
    case ARG_LONG:       *(long *)arg->dest = (value.l);                  break;
    case ARG_ULONG:      *(unsigned long *)arg->dest = (value.ul);        break;
    case ARG_LONG_LONG:  *(long long *)arg->dest = (value.ll);            break;
    case ARG_ULONG_LONG: *(unsigned long long *)arg->dest = (value.ull);  break;
    case ARG_FLOAT:      *(float *)arg->dest = (value.f);                 break;
    case ARG_DOUBLE:     *(double *)arg->dest = (value.d);                break;
    case ARG_STRING:     *(char **)arg->dest = strdup(value.s);           break;
    }
}
// clang-format on

void append_value_to_dest(Argument *arg, ArgumentValue val) {
    int cur = arg->dest_arr_count ? *arg->dest_arr_count : 0;
    int sz = get_argtype_size(arg->type);

    void *newbuf = realloc(*(void **)arg->dest, sz * (cur + 1));
    if (!newbuf) {
        perror("realloc");
        return;
    }
    *(void **)arg->dest = newbuf;
    // clang-format off
    /* записываем значение в конец массива */
    void *slot = (char *)newbuf + cur * sz;
    switch (arg->type) {
    case ARG_BOOL:       *(bool *)slot = val.b;                 break;
    case ARG_CHAR:       *(char *)slot = val.c;                 break;
    case ARG_UCHAR:      *(unsigned char *)slot = val.uc;       break;
    case ARG_INT:        *(int *)slot = val.i;                  break;
    case ARG_UINT:       *(unsigned int *)slot = val.ui;        break;
    case ARG_LONG:       *(long *)slot = val.l;                 break;
    case ARG_ULONG:      *(unsigned long *)slot = val.ul;       break;
    case ARG_LONG_LONG:  *(long long *)slot = val.ll;           break;
    case ARG_ULONG_LONG: *(unsigned long long *)slot = val.ull; break;
    case ARG_FLOAT:      *(float *)slot = val.f;                break;
    case ARG_DOUBLE:     *(double *)slot = val.d;               break;
    case ARG_STRING:     *(char **)slot = strdup(val.s);        break;
    }

    if (arg->dest_arr_count)
        (*arg->dest_arr_count)++;
}
// clang-format on

ArgumentValue strval_to_argval(Argument *arg, char *strval) {
    ArgumentValue v = {0};
    switch (arg->type) {
    case ARG_CHAR:
        v.c = (char)atoi(strval);
        break;
    case ARG_UCHAR:
        v.uc = (unsigned char)atoi(strval);
        break;
    case ARG_INT:
        v.i = atoi(strval);
        break;
    case ARG_UINT:
        v.ui = (unsigned int)strtoul(strval, NULL, 10);
        break;
    case ARG_LONG:
        v.l = strtol(strval, NULL, 10);
        break;
    case ARG_ULONG:
        v.ul = strtoul(strval, NULL, 10);
        break;
    case ARG_LONG_LONG:
        v.ll = strtoll(strval, NULL, 10);
        break;
    case ARG_ULONG_LONG:
        v.ull = strtoull(strval, NULL, 10);
        break;
    case ARG_FLOAT:
        v.f = strtof(strval, NULL);
        break;
    case ARG_DOUBLE:
        v.d = strtod(strval, NULL);
        break;
    case ARG_STRING:
        v.s = strval; /* не strdup — только для чтения */
        break;
    default:
        break;
    }
    return v;
}

// clang-format off
void count_inc(Argument *arg) {

    switch (arg->type) {
    case ARG_CHAR:          (*(char *)(arg->dest))++;               break;
    case ARG_UCHAR:         (*(unsigned char *)(arg->dest))++;      break;
    case ARG_INT:           (*(int *)(arg->dest))++;                break;
    case ARG_UINT:          (*(unsigned int *)(arg->dest))++;       break;
    case ARG_LONG:          (*(long *)(arg->dest))++;               break;
    case ARG_ULONG:         (*(unsigned long *)(arg->dest))++;      break;
    case ARG_LONG_LONG:     (*(long long *)(arg->dest))++;          break;
    case ARG_ULONG_LONG:    (*(unsigned long long *)(arg->dest))++; break;
    }
}
// clang-format on

void parse_args(ArgumentParser *parser, int argc, char **argv) {
    for (int i = 0; i < argc; i++) {
        bool optional = isprefix(parser, argv[i][0]);
        Argument *arg = NULL;
        if (!optional) {
            arg = findnextpositional(parser);
            if (!arg) {
                fprintf(stderr, "error: unexpected positional argument: %s\n", argv[i]);
                if (parser->exit_on_error) {
                    exit(-1);
                }
            } else {
                switch (arg->nargs_type) {
                case NARGS_NONE: {
                    argument_read_value(arg, argv[i], false, 0);
                } break;
                case NARGS_OPTIONAL: {
                    i++;
                    if (i < argc && !findflag(parser, argv[i])) {
                        if (!(*(void **)(arg->dest))) {
                            *((void **)(arg->dest)) = malloc(get_argtype_size(arg->type));
                        }
                        argument_read_value(arg, argv[i], false, 0);
                        arg->has_value = true;
                        *(arg->dest_arr_count) = 1;
                    } else {
                        i--;
                    }
                } break;
                case NARGS_CONST: {
                    if (!(*(void **)(arg->dest))) {
                        *((void **)(arg->dest)) =
                            malloc(get_argtype_size(arg->type) * arg->nargs_count);
                    }
                    for (size_t ii = 0; ii < arg->nargs_count; ii++, i++) {
                        if (i >= argc || findflag(parser, argv[i])) {
                            fprintf(stderr, "error: argument %s: expected exact %d values",
                                    arg->flags[0], arg->nargs_count);
                            if (parser->exit_on_error) {
                                exit(-1);
                            }
                        } else {
                            argument_read_value(arg, argv[i], true, ii);
                        }
                    }
                    i--;
                    arg->has_value = true;
                    if (arg->dest_arr_count) {
                        *(arg->dest_arr_count) = arg->nargs_count;
                    }
                } break;
                case NARGS_ZERO_PLUS: {
                    i++;
                    if (i >= argc || findflag(parser, argv[i])) {
                        i--;
                    } else {
                        if (!(*(void **)(arg->dest))) {
                            *((void **)(arg->dest)) = malloc(get_argtype_size(arg->type));
                        }
                        arg->nargs_count = 0;
                        int offset = 0;
                        while (i < argc && !findflag(parser, argv[i])) {
                            *((void **)(arg->dest)) =
                                realloc(*((void **)(arg->dest)),
                                        get_argtype_size(arg->type) * (arg->nargs_count + 1));
                            argument_read_value(arg, argv[i], true, offset);
                            arg->nargs_count++;
                            offset++;
                            i++;
                        }
                        i--;
                        *(arg->dest_arr_count) = arg->nargs_count;
                    }

                } break;
                case NARGS_ONE_PLUS: {
                    i++;
                    if (i >= argc || findflag(parser, argv[i])) {
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
                            argument_read_value(arg, argv[i], true, offset);
                            arg->nargs_count++;
                            offset++;
                            i++;
                        } while (i < argc && !findflag(parser, argv[i]));
                        i--;
                        *(arg->dest_arr_count) = arg->nargs_count;
                    }

                } break;
                default:
                    break;
                }
                arg->has_value = true;
            }
            continue; // не идём в цикл по флагам
        }

        arg = findflag(parser, argv[i]);
        if (!arg) {
            // возможно, короткий флаг, повторенный N раз, или несколько коротких флагов
            // TODO
            fprintf(stderr, "Unrecongnized argument: %s\n", argv[i]);
            if (parser->exit_on_error) {
                exit(-1);
            }
        }
        // проверям всякое
        if ((arg->action != ACTION_APPEND && arg->has_value) &&
            (arg->action != ACTION_APPEND_CONST && arg->has_value) &&
            (arg->action != ACTION_COUNT)) {
            // перепоявление флага, атата
            fprintf(stderr, "error: argument %s: expected one argument", arg->flags[0]);
            if (parser->exit_on_error) {
                exit(-1);
            }
        }
        switch (arg->action) {
        case ACTION_STORE: {
            if (arg->type == ARG_BOOL) {
                *((bool *)arg->dest) = true;
                arg->has_value = true;
                continue;
            } else {
                if (arg->nargs_type == NARGS_NONE) {
                    i++;
                    if (i >= argc || findflag(parser, argv[i])) {
                        fprintf(stderr, "error: argument %s: expected value", arg->flags[0]);
                        if (parser->exit_on_error) {
                            exit(-1);
                        }
                    } else {
                        /* try parse value from char* to arg->type */
                        argument_read_value(arg, argv[i], false, 0);
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
                            if (i >= argc || findflag(parser, argv[i])) {
                                fprintf(stderr, "error: argument %s: expected exact %d values",
                                        arg->flags[0], arg->nargs_count);
                                if (parser->exit_on_error) {
                                    exit(-1);
                                }
                            } else {
                                argument_read_value(arg, argv[i], true, ii);
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
                        if (i < argc && !findflag(parser, argv[i])) {
                            if (!(*(void **)(arg->dest))) {
                                *((void **)(arg->dest)) = malloc(get_argtype_size(arg->type));
                            }
                            argument_read_value(arg, argv[i], false, 0);
                            arg->has_value = true;
                            *(arg->dest_arr_count) = 1;
                        } else {
                            i--;
                        }
                        break;
                    }
                    case NARGS_ZERO_PLUS: {
                        i++;
                        if (i >= argc || findflag(parser, argv[i])) {
                            i--;
                        } else {
                            if (!(*(void **)(arg->dest))) {
                                *((void **)(arg->dest)) = malloc(get_argtype_size(arg->type));
                            }
                            arg->nargs_count = 0;
                            int offset = 0;
                            while (i < argc && !findflag(parser, argv[i])) {
                                *((void **)(arg->dest)) =
                                    realloc(*((void **)(arg->dest)),
                                            get_argtype_size(arg->type) * (arg->nargs_count + 1));
                                argument_read_value(arg, argv[i], true, offset);
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
                        if (i >= argc || findflag(parser, argv[i])) {
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
                                *((void **)(arg->dest)) = realloc(
                                    *((void **)(arg->dest)),
                                    get_argtype_size(arg->type) *
                                        (arg->nargs_count > 0 ? arg->nargs_count
                                                              : get_argtype_size(arg->type)));
                                argument_read_value(arg, argv[i], true, offset);
                                arg->nargs_count++;
                                offset++;
                                i++;
                            } while (i < argc && !findflag(parser, argv[i]));
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

        } break;
        case ACTION_STORE_TRUE: {
            *(bool *)arg->dest = true;
            arg->has_value = true;
        } break;
        case ACTION_STORE_FALSE: {
            *(bool *)arg->dest = false;
            arg->has_value = true;
        } break;
        case ACTION_STORE_CONST: {
            set_arg_dest(arg, arg->const_val);
            arg->has_value = true;
        } break;
        case ACTION_COUNT: {
            count_inc(arg);
            arg->has_value = true;
        } break;
        case ACTION_APPEND: {
            i++;
            if (i >= argc || findflag(parser, argv[i])) {
                fprintf(stderr, "Флаг есть, а значения нет");
                if (parser->exit_on_error) {
                    exit(-1);
                }
            }

            if (arg->nargs_type != NARGS_NONE) {
                perror("NARGS not supported for ACTION_APPEND");
                exit(-1);
            }
            append_value_to_dest(arg, strval_to_argval(arg, argv[i]));
        } break;
        case ACTION_APPEND_CONST: {
            append_value_to_dest(arg, arg->const_val);
        } break;
        default:
            break;
        }
    }

    bool found = false;
    for (size_t j = 0; j < parser->args_count; j++) {
        Argument *arg = &parser->args[j];
        if (arg->required && !arg->has_value) {
            fprintf(stderr, "Required argument \"%s\" not set", arg->flags[0]);
            if (parser->exit_on_error) {
                exit(-1);
            }
        }
        if (!arg->has_value && arg->action != ACTION_STORE_CONST) {
            set_arg_dest(arg, arg->default_val);
        }
    }
}

typedef struct {
    ArgumentAction action;
    ArgumentType type;
    NargsType nargs;
    bool required;
    bool optional;
    int count;
    int *dest_count;
    ArgumentValue const_val;
    ArgumentValue default_val;
} ArgumentOptions;

void validate_arg_flags(ArgumentParser *parser, Argument *arg) {
    bool need_prefix = false;
    int prefix_count = 0;
    for (size_t i = 0; i < arg->flags_count; i++) {
        char *flag = arg->flags[i];

        need_prefix = isprefix(parser, flag[0]);
        if (need_prefix) {
            prefix_count++;
        }
        if (!arg->is_optional && !need_prefix && i >= 1) {
            fprintf(stderr, "Positional argument should be only one "
                            "given in _flags_ parameter");

            if (parser->exit_on_error) {
                exit(-1);
            }
        }
    }
    if (need_prefix && prefix_count != arg->flags_count) {

        fprintf(stderr, "All flags must start with prefix");
        if (parser->exit_on_error) {
            exit(-1);
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
    if (arg->flags_count == 1 && !isprefix(parser, arg->flags[0][0])) {
        arg->positional = true;
    }

    arg->dest = dest;
    arg->dest_arr_count = opt.dest_count;
    arg->type = opt.type;
    arg->action = opt.action;
    if (opt.required) {
        arg->required = true;
        arg->is_optional = false;
    }
    if (opt.optional) {
        arg->required = false;
        arg->is_optional = true;
    }
    if (!opt.required && !opt.optional) {
        arg->required = arg->positional;
        arg->is_optional = !arg->positional;
    }

    arg->nargs_type = opt.nargs;
    if (opt.nargs > 0) { // CONST

        arg->nargs_type = NARGS_CONST;
        if (opt.count) {
            arg->nargs_count = opt.count;
        } else {
            arg->nargs_count = opt.nargs;
        }
    }
    arg->const_val = opt.const_val;
    arg->default_val = opt.default_val;
    if (arg->type == ARG_BOOL && arg->nargs_type != NARGS_NONE) { // массив булов, ай ай ай
        fprintf(stderr, "error: boolean flags can't contain any values");
        if (parser->exit_on_error) {
            exit(-1);
        }
    }

    if (arg->action == ACTION_COUNT && (arg->nargs_type != NARGS_NONE || arg->nargs_count != 0)) {
        fprintf(stderr, "error: countable flags cant store nargs values");
        if (parser->exit_on_error) {
            exit(-1);
        }
    }

    if (arg->positional && (arg->action != ACTION_STORE &&
                            arg->action != ACTION_APPEND)) { // wrong action for positional argument
        fprintf(stderr,
                "error: wrong action for positional argument, supports only STORE and APPEND");
        if (parser->exit_on_error) {
            exit(-1);
        }
    }
}

#endif // SHIT_ARGPARSE