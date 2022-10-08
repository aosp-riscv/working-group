#include <stdio.h>

__asm__(".symver foo_old,foo@VER_1");
__asm__(".symver foo_new,foo@@VER_2");

void foo_old(void)
{
	printf("v1 foo\n");
}

void foo_new(void)
{
	printf("v2 foo\n");
}
