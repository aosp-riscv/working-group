#include <stdio.h>

void func(void)
{
    void *ra = __builtin_return_address(0);
    void *fp = __builtin_frame_address(0);
    printf("func: ra = %p, fp = %p\n", ra, fp);

    ra = __builtin_return_address(1);
    fp = __builtin_frame_address(1);
    printf("func: ra = %p, fp = %p\n", ra, fp);
}

int main(void)
{
    void *ra = __builtin_return_address(0);
    void *fp = __builtin_frame_address(0);
    printf("main: ra = %p, fp = %p\n", ra, fp);

    func();

    return 0;
}