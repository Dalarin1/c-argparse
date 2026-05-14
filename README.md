# SHIT - ARGPARSE

Simple C library for parsing command-line arguments in Python argparse style.

## Usage

```c
#include "argparse.h"

int main(int argc, char **argv) {
    ArgumentParser parser = parser_new();


    // single optional string flag
    char *name = NULL;
    add_argument(parser, "-n --name", &name);

    // single positional string parameter
    char* oleg = NULL;
    add_argument(parser, "oleg", &oleg);

    // optional bool flag
    bool yes;
    add_argument(parser, "-y", &yes, .type=ARG_BOOL);
    
    // optional integer array flag with const lenght
    int* ages_arr = NULL;
    add_argument(parser, "-a --ages -aa ---ag", &ages_arr, 
                .type=ARG_INT,
                .nargs=5);

    // optional float array flag with variadic length with elements count >= 1
    // example:
    // main -b 11.0 will end with biases_cont = 1
    // main -b 11.0 12.0 will end with biases_count = 2
    // main -b will drop with error

    float* biases = NULL;
    int biases_count = 0;
    add_argument(parser, "-b -biases --bb", &ages_arr, 
                .type=ARG_FLOAT,
                .nargs=NARGS_ONE_PLUS,
                .dest_count = &biases_count);

    parse_args(parser, argc - 1, argv + 1);

    parser_free(parser);
}
```

## API Reference

### parser_new()

```c
ArgumentParser *parser_new(const char *prog, const char *description);
```

- `prog` — program name (can be NULL)
- `description` — description (can be NULL)

### add_argument()

```c
add_argument(
    ArgumentParser *parser,
    const char *flags,      // space-delimited: "-n --name"
    void *dest,             // where to store the result
    ...                     // optional parameters
)
```

**Optional parameters:**

| Parameter | Type | Default | Description |
| ---------- | ----- | -------------- | ---------- |
| `.type` | `ArgumentType` | `ARG_STRING` | Argument type |
| `.action` | `ArgumentAction` | `ACTION_STORE` | Parsing action |
| `.nargs` | `NargsType` or `int` | `NARGS_NONE` | Number of values |
| `.required` | `bool` | `false` | Whether the argument is required |
| `.count` | `int` | `0` | Exact number of values (for `NARGS_CONST`) |
| `.dest_count` | `int *` | `NULL` | Where to write the actual number of values |

### ArgumentType

```c
ARG_BOOL        
ARG_CHAR        
ARG_UCHAR       
ARG_INT         
ARG_UINT       
ARG_LONG        
ARG_ULONG        
ARG_LONG_LONG    
ARG_ULONG_LONG   
ARG_STRING      
ARG_FLOAT       
ARG_DOUBLE      
```

### NargsType

```c
NARGS_NONE          // single value (default)
NARGS_CONST         // exact count (specified via .count)
NARGS_ZERO_PLUS     // 0 or more (*)
NARGS_ONE_PLUS      // 1 or more (+)
NARGS_OPTIONAL      // 0 or 1 (?)
```

Or you can specify an integer `>= 0` directly in `.nargs=5`.

### ArgumentAction

```c
ACTION_STORE         // store value (default)
ACTION_STORE_CONST   
ACTION_STORE_TRUE    
ACTION_STORE_FALSE   
ACTION_APPEND        // allows specifying flag multiple times
ACTION_APPEND_CONST  
ACTION_COUNT         // count occurrences (-v -v -v → 3)
ACTION_HELP          
ACTION_VERSION       
```

## Examples

### Simple optional flag

```c
char *name = NULL;
add_argument(parser, "-n --name", &name);
// ./app -n Alice
// name = "Alice"
```

### Positional argument

```c
char *filename = NULL;
add_argument(parser, "filename", &filename);
// ./app input.txt
// filename = "input.txt"
```

### Boolean flag

```c
bool verbose = false;
add_argument(parser, "-v --verbose", &verbose, .type=ARG_BOOL);
// ./app -v
// verbose = true
```

### Fixed-length array

```c
int *coords = NULL;
add_argument(parser, "-c --coords", &coords, .type=ARG_INT, .nargs=3);
// ./app -c 10 20 30
// coords[0]=10, coords[1]=20, coords[2]=30
```

### Variable-length array (1+)

```c
float *values = NULL;
int count = 0;
add_argument(parser, "-v --values", &values, 
    .type=ARG_FLOAT,
    .nargs=NARGS_ONE_PLUS,
    .dest_count=&count
);
// ./app -v 1.5 2.3 4.8
// values[] = {1.5, 2.3, 4.8}, count = 3
```

### Variable-length array (0+)

```c
char **files = NULL;
int files_count = 0;
add_argument(parser, "-f --files", &files,
    .type=ARG_STRING,
    .nargs=NARGS_ZERO_PLUS,
    .dest_count=&files_count
);
// ./app -f a.txt b.txt
// files[] = {"a.txt", "b.txt"}, files_count = 2
// ./app
// files = NULL, files_count = 0
```

## Important Notes

### Memory Management

- **Strings (`ARG_STRING`)** are automatically copied via `strdup()` — **you are responsible for freeing them**
- **Arrays** are allocated via `malloc/realloc` — **you are responsible for freeing them**
- `parser_free()` only frees internal parser structures, **not user variables**

```c
char *name = NULL;
int *ages = NULL;

add_argument(parser, "-n", &name);
add_argument(parser, "-a", &ages, .type=ARG_INT, .nargs=3);
parse_args(parser, argc - 1, argv + 1);

// your responsibility:
free(name);
free(ages);

parser_free(parser);  // only frees the parser
```

### Positional argument order

Positional arguments are parsed **in the order they are added** via `add_argument()`:

```c
add_argument(parser, "source", &src);     // first
add_argument(parser, "dest", &dst);       // second
// ./app file1.txt file2.txt
// src="file1.txt",
// dst="file2.txt"
```

### Error Handling

By default, the program exits via `exit(-1)` on error. This is controlled by the `parser->exit_on_error` field (default `true`).
