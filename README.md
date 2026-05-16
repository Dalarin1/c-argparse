# SHIT - ARGPARSE

Simple C library for parsing command-line arguments in Python argparse style.
For education purposes only. (It's too bad for production)

## Usage

```c
#include "argparse.h"

int main(int argc, char **argv) {
    ArgumentParser *parser = parser_new("app", "Demo application");

    char *input = NULL;

    /* positional argument */
    add_argument(parser, "input", &input,
        .type = ARG_STRING
    );

    /*  */
    char** files = NULL;
    add_argument(parser, "-f --path", &files,
        .type=ARG_STRING,
        .action = ACTION_APPEND);
    
    long count = 0;
    add_argument(parser, "-c --count", &count,
        .type=ARG_LONG,
        .action=ACTION_STORE_CONST,
        .const_val.l = 101112);
    
    /* array of variadic length */
    int* numbers = NULL;
    int numbers_count = 0;
    add_argument(parser, "numbers", &numbers,
        .type= ARG_INT,
        .nargs= NARGS_ZERO_PLUS,
        .dest_count= &numbers_count);

    parse_args(parser, argc - 1, argv + 1);

    parser_free(parser);

    free(input);
    free(files);
    free(numbers);

    return 0;
}

```

### Notes

Parser DO allocate memory for storing parsed arrays.
However, that's YOU who must free it after usage
