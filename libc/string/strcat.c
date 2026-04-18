#include <string.h>

char* strcat(char* __restrict dest, const char* __restrict src)
{
	char* d = dest;
	while (*d) {
		d++;
	}
	while (*src) {
		*d++ = *src++;
	}
	*d = '\0';
	return dest;
}
