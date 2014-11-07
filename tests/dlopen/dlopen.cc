#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, const char *argv[])
{
	void *handle;
	int (*sym)(int);

	handle = dlopen("libshared_library.so", RTLD_LAZY);
	if (!handle) {
		printf("Can't dlopen\n");
		exit(1);
	}

	dlerror();
	sym = (int (*)(int))dlsym(handle, "vobb");
	if (!sym) {
		printf("No symbol\n");
		dlclose(handle);
		exit(1);
	}

	int a = sym(5);
	printf("from shared lib: %d\n", a);

	dlclose(handle);

	return 0;
}
