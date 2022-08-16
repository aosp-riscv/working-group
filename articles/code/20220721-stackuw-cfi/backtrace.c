#include <stdio.h>
#include <stdlib.h>
#include <execinfo.h>

// referring to EXAMPLE from `man 3 backtrace`

#define BT_BUF_SIZE 100

void unwind_by_backtrace() 
{
    int nptrs;
    void *buffer[BT_BUF_SIZE];

    nptrs = backtrace(buffer, BT_BUF_SIZE);

    char **strings = backtrace_symbols(buffer, nptrs);
    for (int i = 0; i < nptrs; ++i) {
        printf("%s\n", strings[i]);
    }
    free(strings);
}
