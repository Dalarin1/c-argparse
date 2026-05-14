# SHIT - ARGPARSE

Простая C библиотека для парсинга аргументов командной строки в стиле Python argparse.

## Использование

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

- `prog` — имя программы (может быть NULL)
- `description` — описание (может быть NULL)

### add_argument()

```c
add_argument(
    ArgumentParser *parser,
    const char *flags,      // через пробел: "-n --name"
    void *dest,             // куда сохранить результат
    ...                     // опциональные параметры
)
```

**Опциональные параметры:**

| Параметр | Тип | По умолчанию | Описание |
| ---------- | ----- | -------------- | ---------- |
| `.type` | `ArgumentType` | `ARG_STRING` | Тип аргумента |
| `.action` | `ArgumentAction` | `ACTION_STORE` | Действие при парсинге |
| `.nargs` | `NargsType` или `int` | `NARGS_NONE` | Количество значений |
| `.required` | `bool` | `false` | Обязателен ли аргумент |
| `.count` | `int` | `0` | Точное количество значений (для `NARGS_CONST`) |
| `.dest_count` | `int *` | `NULL` | Куда записать фактическое количество значений |

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
NARGS_NONE          // одно значение (default)
NARGS_CONST         // точное количество (задаётся через .count)
NARGS_ZERO_PLUS     // 0 или более (*)
NARGS_ONE_PLUS      // 1 или более (+)
NARGS_OPTIONAL      // 0 или 1 (?)
```

Или можно указать целое число `>= 0` напрямую в `.nargs=5`.

### ArgumentAction

```c
ACTION_STORE         // сохранить значение (default)
ACTION_STORE_CONST   
ACTION_STORE_TRUE    
ACTION_STORE_FALSE   
ACTION_APPEND        // позволяет указывать флаг несколько раз
ACTION_APPEND_CONST  
ACTION_COUNT         // подсчитать количество (-v -v -v → 3)
ACTION_HELP          
ACTION_VERSION       
```

## Примеры

### Простой optional флаг

```c
char *name = NULL;
add_argument(parser, "-n --name", &name);
// ./app -n Alice
// name = "Alice"
```

### Positional аргумент

```c
char *filename = NULL;
add_argument(parser, "filename", &filename);
// ./app input.txt
// filename = "input.txt"
```

### Boolean флаг

```c
bool verbose = false;
add_argument(parser, "-v --verbose", &verbose, .type=ARG_BOOL);
// ./app -v
// verbose = true
```

### Массив фиксированной длины

```c
int *coords = NULL;
add_argument(parser, "-c --coords", &coords, .type=ARG_INT, .nargs=3);
// ./app -c 10 20 30
// coords[0]=10, coords[1]=20, coords[2]=30
```

### Массив переменной длины (1+)

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

### Массив переменной длины (0+)

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

### Комплексный пример

```c
int main(int argc, char **argv) {
    ArgumentParser *parser = parser_new("converter", "File format converter");

    char *input = NULL;
    char *output = NULL;
    bool verbose = false;
    int *dimensions = NULL;
    
    add_argument(parser, "input", &input);
    add_argument(parser, "output", &output);
    add_argument(parser, "-v --verbose", &verbose, .type=ARG_BOOL);
    add_argument(parser, "-d --dimensions", &dimensions, .type=ARG_INT, .nargs=2);

    parse_args(parser, argc - 1, argv + 1);

    printf("Input: %s\n", input);
    printf("Output: %s\n", output);
    if (verbose) printf("Verbose mode ON\n");
    if (dimensions) printf("Dimensions: %dx%d\n", dimensions[0], dimensions[1]);

    parser_free(parser);
    return 0;
}
```

```bash
$ ./converter input.png output.jpg -v -d 800 600
Input: input.png
Output: output.jpg
Verbose mode ON
Dimensions: 800x600
```

## Важные замечания

### Управление памятью

- **Строки (`ARG_STRING`)** автоматически копируются через `strdup()` — **вы отвечаете за их освобождение**
- **Массивы** выделяются через `malloc/realloc` — вы **отвечаете за их освобождение**
- `parser_free()` освобождает только внутренние структуры парсера, **не пользовательские переменные**

```c
char *name = NULL;
int *ages = NULL;

add_argument(parser, "-n", &name);
add_argument(parser, "-a", &ages, .type=ARG_INT, .nargs=3);
parse_args(parser, argc - 1, argv + 1);

// ваша работа:
free(name);
free(ages);

parser_free(parser);  // освобождает только парсер
```

### Порядок positional аргументов

Positional аргументы парсятся **в порядке добавления** через `add_argument()`:

```c
add_argument(parser, "source", &src);     // первый
add_argument(parser, "dest", &dst);       // второй
// ./app file1.txt file2.txt
// src="file1.txt",
// dst="file2.txt"
```

### Обработка ошибок

По умолчанию при ошибке программа завершается через `exit(-1)`. Это контролируется полем `parser->exit_on_error` (по умолчанию `true`).
