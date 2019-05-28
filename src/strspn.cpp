#include "asmlib.h"
#include "asmlib-internal.h"
#include <smmintrin.h>

extern "C" size_t strspnSSE42(const char *string, const char *set) __attribute__((target("sse4.2")));
extern "C" size_t strspnSSE42(const char *string, const char *set)
{
	size_t spanCounter = 0;
	const __m128i *sseString = (const __m128i *)string;
	const __m128i *sseSet = (const __m128i *)set;

	while (1)
	{
		auto currentSetBytes = _mm_loadu_si128(sseSet);
		auto currentStringBytes = _mm_loadu_si128(sseString);
		auto resultBitMask = _mm_cvtsi128_si32(_mm_cmpistrm(currentSetBytes, currentStringBytes, 0));	// Find in set, return bit mask
		if (!_mm_cmpistrs(currentSetBytes, currentStringBytes, 0))
		{
			while (1)
			{
				++sseSet;
				currentSetBytes = _mm_loadu_si128(sseSet);
				auto tmpResultBitMask = _mm_cvtsi128_si32(_mm_cmpistrm(currentSetBytes, currentStringBytes, 0));	// find in set, return bit mask
				if (_mm_cmpistrs(currentSetBytes, currentStringBytes, 0))
				{
					sseSet = (const __m128i *)set;
					resultBitMask |= tmpResultBitMask;
					break;
				}
				resultBitMask |= tmpResultBitMask;
			}
		}
		if ((uint16_t)resultBitMask != (uint16_t)-1)
			return asmlibInternal::bsf(~resultBitMask) + spanCounter;

		++sseString;
		spanCounter += 16;
	}
}

extern "C" size_t strcspnSSE42(const char *string, const char *set) __attribute__((target("sse4.2")));
extern "C" size_t strcspnSSE42(const char *string, const char *set)
{
	size_t spanCounter = 0;
	const __m128i *sseString = (const __m128i *)string;
	const __m128i *sseSet = (const __m128i *)set;

	while (1)
	{
		auto currentSetBytes = _mm_loadu_si128(sseSet);
		auto currentStringBytes = _mm_loadu_si128(sseString);
		auto resultBitMask = _mm_cvtsi128_si32(_mm_cmpistrm(currentSetBytes, currentStringBytes, 0b110000));	// Find in set, invert valid bits, return bit mask
		if (!_mm_cmpistrs(currentSetBytes, currentStringBytes, 0b110000))
		{
			while (1)
			{
				++sseSet;
				currentSetBytes = _mm_loadu_si128(sseSet);
				auto tmpResultBitMask = _mm_cvtsi128_si32(_mm_cmpistrm(currentSetBytes, currentStringBytes, 0b110000));	// find in set, invert valid bits, return bit mask
				if (_mm_cmpistrs(currentSetBytes, currentStringBytes, 0b110000))
				{
					sseSet = (const __m128i *)set;
					resultBitMask |= tmpResultBitMask;
					break;
				}
				resultBitMask |= tmpResultBitMask;
			}
		}
		if ((uint16_t)resultBitMask != (uint16_t)-1)
			return asmlibInternal::bsf(~resultBitMask) + spanCounter;

		++sseString;
		spanCounter += 16;
	}
}

extern "C" size_t strspnGeneric(const char *string, const char *set)
{
	auto startStringPtr = string;

	while (1)
	{
		auto currentStringCharacter = *string;
		if (!currentStringCharacter)
			return string - startStringPtr;

		char currentSetCharacter;
		do
		{
			currentSetCharacter = *set;
			if (!currentSetCharacter)
				return string - startStringPtr;

			++set;
		} while (currentStringCharacter != currentSetCharacter);
		++string;
	}
}

extern "C" size_t strcspnGeneric(const char *string, const char *set)
{
	auto startStringPtr = string;

	while (1)
	{
		auto currentStringCharacter = *string;
		if (!currentStringCharacter)
			return string - startStringPtr;

		char currentSetCharacter;
		while (1)
		{
			currentSetCharacter = *set;
			if (!currentSetCharacter)
				break;

			++set;

			if (currentStringCharacter == currentSetCharacter)
				return string - startStringPtr;	// Character match found, stop search
		};
		++string;	// End of set, mismatch found
	}
}

static size_t strspnCPUDispatch(const char *string, const char *set);
auto strspnDispatch = strspnCPUDispatch;

static size_t strcspnCPUDispatch(const char *string, const char *set);
auto strcspnDispatch = strcspnCPUDispatch;

static size_t strspnCPUDispatch(const char *string, const char *set)
{
	auto result = strspnGeneric;
	if (InstructionSet() >= asmlibInternal::InstructionSetReturnValues::sse42Supported)
		result = strspnSSE42;

	strspnDispatch = result;

	return result(string, set);
}

static size_t strcspnCPUDispatch(const char *string, const char *set)
{
	auto result = strcspnGeneric;
	if (InstructionSet() >= asmlibInternal::InstructionSetReturnValues::sse42Supported)
		result = strcspnSSE42;

	strcspnDispatch = result;

	return result(string, set);
}

extern "C" size_t A_strspn(const char *string, const char *set)
{
	return strspnDispatch(string, set);
}

extern "C" size_t A_strcspn(const char *string, const char *set)
{
	return strcspnDispatch(string, set);
}
