#include "asmlib.h"

extern "C" int A_stricmp(const char *str1, const char *str2)
{
	size_t offset = str2 - str1;
	char currentChar;
	do
	{
		currentChar = *str1;
		if (currentChar != *(str1 + offset))
		{
			// Bytes are different, check case
			currentChar ^= 0x20;
			if (currentChar != *(str1 + offset) || (((currentChar | 0x20) - 'a') >= 'z' - 'a'))
			{
				// Bytes are different, even after changing case
				currentChar = *str1;	// Get original value again
				currentChar -= 'A';
				if (currentChar >= ('Z' - 'A'))
					currentChar += 0x20;

				char currentChar2 = *(str1 + offset);
				currentChar2 -= 'A';
				if (currentChar2 >= ('Z' - 'A'))
					currentChar2 += 0x20;

				return currentChar - currentChar2;	// Subtract to get result
			}
		}
		++str1;
	} while (currentChar);

	return 0;	// Terminating 0 found, strings are equal
}
