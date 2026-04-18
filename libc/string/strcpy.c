#include <string.h>

char* strcpy(char* __restrict dest, const char* __restrict src)
{
	char* d = dest;
	while (*src) {
		*d++ = *src++;
	}
	*d = '\0';
	return dest;
}
