__attribute__((weak)) extern int mysym;

int bar()
{
	return mysym;
}
