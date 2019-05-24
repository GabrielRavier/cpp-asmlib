#include "asmlib.h"
#include <smmintrin.h>

int strcmpSSE42(const char *str1, const char *str2) __attribute__((target("sse4.2")));
int strcmpSSE42(const char *str1, const char *str2)
{
	const __m128i *sseStr1 = (const __m128i *)str1;
	const __m128i *sseStr2 = (const __m128i *)str2;
	size_t offset = -1;

	while (1)
	{
		++offset;
		auto currentStr1Bytes = _mm_loadu_si128(sseStr1 + offset);
		auto currentStr2Bytes = _mm_loadu_si128(sseStr2 + offset);
		auto index = _mm_cmpistri(currentStr1Bytes, currentStr2Bytes, 0b11000);
		if (!_mm_cmpistrc(currentStr1Bytes, currentStr2Bytes, 0b11000) && !_mm_cmpistrz(currentStr1Bytes, currentStr2Bytes, 0b11000))	// If properly optimized, should use jnbe after previous usage of _mm_cmpstri
			continue;

		if (!_mm_cmpistrc(currentStr1Bytes, currentStr2Bytes, 0b11000))	// If properly optimized, should use jnc after previous jnbe
			return 0;	// Equal

		// Not equal
		auto actualOffset = (offset * 16) + index;
		return (uint8_t)str1[actualOffset] - (uint8_t)str2[actualOffset];
	}
}

int strcmpGeneric(const char *str1, const char *str2)
{
	while (1)
	{
		auto currentStr1Char = *str1;

		if (currentStr1Char != *str2)
			return (uint8_t)currentStr1Char - (uint8_t)*str2;

		if (!currentStr1Char)
			return 0;

		++str1;
		++str2;
	}
}

static int strcmpCPUDispatch(const char *str1, const char *str2);
auto strcmpDispatch = strcmpCPUDispatch;

static int strcmpCPUDispatch(const char *str1, const char *str2)
{
	auto instructionSet = InstructionSet();
	auto result = strcmpGeneric;

	if (instructionSet >= 10)
		result = strcmpSSE42;

	strcmpDispatch = result;

	return result(str1, str2);
}

extern "C" int A_strcmp(const char *str1, const char *str2)
{
	return strcmpDispatch(str1, str2);
}
