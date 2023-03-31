#include <stdio.h>
#include <elf.h>

int main(int argc, char* argv[], char* envp[])
{
	Elf64_auxv_t *auxv;
	int i;

	printf("argc = %d\n", argc);
	
	for (i = 0; i < argc; i++) {
		printf("argv[%d] = 0x%p -> %s\n", i, argv[i], argv[i]);
	}

	/* from stack diagram above: *envp = NULL marks end of envp */
	i = 0;
	while (NULL != envp[i]) {
		printf("envp[%d] = 0x%p -> %s\n", i, envp[i], envp[i]);
		i++;
	};
	envp += (i + 1);

	/* auxv->a_type = AT_NULL marks the end of auxv */
	for (auxv = (Elf64_auxv_t *)envp; auxv->a_type != AT_NULL; auxv++) {
		printf("AT: type = %ld, value = 0x%lx\n", auxv->a_type, auxv->a_un.a_val);
	}

	return 0;
}
