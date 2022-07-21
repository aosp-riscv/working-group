#include <stdio.h>
#include <stdlib.h>
#include <execinfo.h>

void unwind_by_backtrace() 
{
    const int backtrace_max_size = 100;
    void *buffer[backtrace_max_size];

    int buffer_size = backtrace(buffer, backtrace_max_size);

    char **symbols = backtrace_symbols(buffer, buffer_size);
    for (int i = 0; i < buffer_size; ++i) {
        printf("%p:%s\n", buffer[i], symbols[i]);
    }
    free(symbols);
}
