# SHIT - ARGPARSE

## Usage

```c
int main(int argc, char **argv) {
    ArgumentParser parser = parser_new();

    int optional_int;
    add_argument(parser, "-s --single", ARG_INT, &optional_int); // optional argument

    char *positional_string = NULL; // All pointer-type vars should be NULLed, or ...
    add_argument(parser, "stringini", ARG_STRING, &positional_string);

    char **optional_list_of_strings = NULL;
    int expected_strings_count = 3;
    add_argument_n(parser, "-l -list --wololo ---das_ist_flag", ARG_STRING,
                   &optional_list_of_strings, NARGS_CONST, expected_strings_count, NULL);

    char *file = NULL;
    int files_count = 0;
    add_argument_n(parser, "--file -f", ARG_STRING, &file, NARGS_OPTIONAL, 0, &files_count);

    char **names = NULL;
    int names_count = 0;
    add_argument_n(parser, "-n --names", ARG_STRING, &names, NARGS_ZERO_PLUS, 0, &names_count);

    int *ages = NULL;
    int ages_count;
    add_argument_n(parser, "-a --ages", ARG_INT, &ages, NARGS_ONE_PLUS, 0, &ages_count);

    parse_args(argc - 1, argv + 1);

    parser_free(parser);
}
```
