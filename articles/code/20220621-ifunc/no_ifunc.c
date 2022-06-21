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

int myfunc(int a)
{
    printf("myfunc is called\n");
    if (1 == get_configure()) {
        return myfunc_1(a);
    } else {
        return myfunc_2(a);
    }
}

void test_myfunc()
{
    for (int i = 0; i < 3; i++) {
        printf("myfunc returns %d\n", myfunc(1));
    }
}
