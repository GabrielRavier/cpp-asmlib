#include "asmlib.h"
#include <cstring>
#include <cstddef>
#include <iostream>
#include <string>
#include <string_view>
#include <cassert>

int main()
{
	const char *testStr = "abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ 1234567890 @`'{}[]()<>";
	constexpr size_t strSize = 0x100;
	char str1[strSize], str2[strSize];

	strcpy(str1, testStr);
	memcpy(str2, str1, strSize);

	// Change case
	str2[4] ^= 0x20;
	str1[30] ^= 0x20;

	assert(A_stricmp(str1, str2) == 0);

	// Make strings different
	str2[8] += 2;

	assert(A_stricmp(str1, str2) < 0);

	// Make strings different
	str2[7] -= 2;
	assert(A_stricmp(str1, str2) > 0);
}
