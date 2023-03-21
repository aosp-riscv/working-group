extern __attribute__((weak)) int mysym;

int bar()
{
	return mysym;
}
