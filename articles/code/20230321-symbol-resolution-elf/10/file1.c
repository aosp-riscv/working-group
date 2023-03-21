extern __attribute__((weak)) int mysym;

int foo()
{
	return mysym;
}
