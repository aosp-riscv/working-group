#include <stdio.h>

extern void test_myfunc();

#ifndef MY_CONFIG
#define MY_CONFIG 1
#endif

static int g_configure = MY_CONFIG;

int get_configure()
{
    return g_configure;
}

int main()
{
    printf("main ......\n");
    test_myfunc();
    return 0;
}