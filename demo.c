#include "argparse.h"

int main(int argc, char **argv) {
    ArgumentParser *parser = parser_new("app", "Demo application");

    /* positional argument */
    char *input = NULL;
    add_argument(parser, "input", &input, .type = ARG_STRING, .action = ACTION_STORE);

    /* append multiple strings */
    char **files = NULL;
    int files_count = 0;
    add_argument(parser, "-f --path", &files, .type = ARG_STRING, .action = ACTION_APPEND,
                 .dest_count = &files_count);

    /* constant value flag */
    long count = 0;
    add_argument(parser, "-c --count", &count, .type = ARG_LONG, .action = ACTION_STORE_CONST,
                 .const_val.l = 101112);

    /* variadic positional array */
    int *numbers = NULL;
    int numbers_count = 0;

    add_argument(parser, "numbers", &numbers, .type = ARG_INT, .nargs = NARGS_ONE_PLUS,
                 .dest_count = &numbers_count, .action = ACTION_STORE);
    
    /* some single-letter flags for -bn */
    bool ba = false, bb = false;;
    add_argument(parser, "-b --ba", &ba, .type=ARG_BOOL);
    add_argument(parser, "-n --bb", &bb, .type=ARG_BOOL);
    
    /* now  supports -vvv */
    int verbose = 0;
    add_argument(parser, "-v --verbose", &verbose, .type=ARG_INT, .action=ACTION_COUNT);

    /* parse CLI */
    parse_args(parser, argc - 1, argv + 1);

    /* results */
    printf("input = %s\n", input);
    printf("count = %ld\n", count);

    printf("files: %d\n", files_count);
    for (int i = 0; i < files_count; i++) {
        printf("  %s\n", files[i]);
    }

    printf("numbers (%d):\n", numbers_count);
    for (int i = 0; i < numbers_count; i++) {
        printf("  %d\n", numbers[i]);
    }
    printf("Ba = %d\n", ba);
    printf("Bb = %d\n", bb);
    printf("Verbosity level = %d\n", verbose);
    /* cleanup */
    parser_free(parser);

    free(input);
    for (int i = 0; i < files_count; i++) {
        free(files[i]);
    }
    free(files);
    free(numbers);

    return 0;
}