#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

extern int get_configure(void);

static int myfunc_1 (int a)
{
    printf("myfunc_1 is called\n");
    return a + 100;
}

static int myfunc_2 (int a){
    printf("myfunc_2 is called\n");
    return a + 200;
}

#ifndef IFUNC_PREEMPTIBLE
static
#endif
int myfunc(int) __attribute__((ifunc ("myfunc_resolver")));

//              myfunc_resolver              -- a symbol to
//              myfunc_resolver(void)        -- a function taking no arguments
//             *myfunc_resolver(void)        -- and returning a pointer
//            (*myfunc_resolver(void)) (int) -- to a function taking an argument of int
//        int (*myfunc_resolver(void)) (int) -- and returning int
// static int (*myfunc_resolver(void)) (int) -- and is not exported to the linker
// so myfunc_resolver, as a function itself, with no argument, will return
// a pointer to a function, which protype is "int func(int)"
int (*myfunc_resolver(void))(int)
{
    printf("myfunc_resolver is called\n");
    if (1 == get_configure()) {
        return myfunc_1;
    } else {
        return myfunc_2;
    }
}

void test_myfunc()
{
    for (int i = 0; i < 3; i++) {
        printf("myfunc returns %d\n", myfunc(1));
    }
}
