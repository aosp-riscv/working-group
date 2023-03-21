__attribute__((weak)) extern int mysym;

int foo()
{
	return mysym;
}
