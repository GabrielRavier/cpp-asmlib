#include "asmlib.h"

extern "C" char *A_strcpy(char *dest, const char *src)
{
	return (char *)A_memcpy(dest, src, A_strlen(src) + 1);
}
