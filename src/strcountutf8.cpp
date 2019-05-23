#include "asmlib.h"
#include <cstdint>
#include <smmintrin.h>

constexpr uint8_t byteRangeData[16] =
{
	0b10000000, 0b10111111,	// Range for UTF-8 continuation bytes
	0b10000000, 0b10111111,	// 8 times for SSE value
	0b10000000, 0b10111111,
	0b10000000, 0b10111111,
	0b10000000, 0b10111111,
	0b10000000, 0b10111111,
	0b10000000, 0b10111111,
	0b10000000, 0b10111111
};

size_t strcount_UTF8SSE42(const char *str) __attribute__((target("sse4.2")));
size_t strcount_UTF8SSE42(const char *str)
{
	const __m128i *sseStr = (const __m128i *)str;
	const __m128i byteRange = _mm_load_si128((const __m128i *)byteRangeData);	// Range of continuation bytes to ignore
	size_t characterCount = 0;

	while (1)
	{
		uint32_t convertedCompResult = _mm_cvtsi128_si32(_mm_cmpistrm(byteRange, *sseStr, 0b110100)); // Check range, invert valid bits, return bit mask
		if (_mm_cmpistrz(byteRange, *sseStr, 0b110100))	// If properly optimized, will use result from previous _mm_cmpistrm call
			return characterCount + _mm_popcnt_u32(convertedCompResult);	// Terminating 0 found

		characterCount += _mm_popcnt_u32(convertedCompResult);
		++sseStr;
	}
}

size_t strcount_UTF8Generic(const char *str)
{
	size_t characterCount = 0;

	while (1)
	{
		uint8_t currentChar = *str;	// One byte from string
		if (!currentChar)	// Terminating 0
			break;

		currentChar -= 0b10000000;	// Lower limit of continuation bytes
		if (currentChar > 0b111111)
			++characterCount;	// Add 1 to character count if above
		++str;
	}

	return characterCount;
}

static size_t strcount_UTF8CPUDispatch(const char *str);
static auto strcount_UTF8Dispatch = strcount_UTF8CPUDispatch;

static size_t strcount_UTF8CPUDispatch(const char* str)
{
	auto instructionSet = InstructionSet();
	auto dispatch = strcount_UTF8Generic;

	if (instructionSet >= 10)	// SSE4.2 supported
		dispatch = strcount_UTF8SSE42;

	strcount_UTF8Dispatch = dispatch;

	return dispatch(str);
}

extern "C" size_t strcount_UTF8(const char *str)
{
	return strcount_UTF8Dispatch(str);
}
