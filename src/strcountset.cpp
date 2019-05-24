#include "asmlib.h"
#include <smmintrin.h>

size_t strCountInSetSSE42(const char *str, const char *set) __attribute__((target("sse4.2")));
size_t strCountInSetSSE42(const char *str, const char *set)
{
	const __m128i *sseStr = (const __m128i *)str;
	const __m128i *sseSetIterator = (const __m128i *)set;
	size_t matchCounter = 0;	// Match counter
	uint32_t tmpMatches;
	uint32_t tmpMatches2;
	__m128i currentSetBytes, currentStrBytes;

	while (1)
	{
		currentStrBytes = _mm_loadu_si128(sseStr);
		currentSetBytes = _mm_loadu_si128(sseSetIterator);

		tmpMatches = _mm_cvtsi128_si32(_mm_cmpistrm(currentSetBytes, currentStrBytes, 0));	// Find in set, return bit mask
		if (!_mm_cmpistrs(currentSetBytes, currentStrBytes, 0))	// If properly optimized, will use result from previous _mm_cmpistrm call
			goto set_extends;	// The set is more than 16 bytes
		if (_mm_cmpistrz(currentSetBytes, currentStrBytes, 0))	// If properly optimized, will use result from previous _mm_cmpistrm call
			return matchCounter + _mm_popcnt_u32(tmpMatches);

set_finished:
		matchCounter += _mm_popcnt_u32(tmpMatches);

		// First 16 characters, continue with next 16 characters (a terminating zero would never match)
		++sseStr;	// Next 16 bytes of str
	}

set_loop:
	tmpMatches |= tmpMatches2;	// Accumulate matches

set_extends:
	++sseSetIterator;
	currentSetBytes = _mm_loadu_si128(sseSetIterator);	// Next part of set
	tmpMatches2 = _mm_cvtsi128_si32(_mm_cmpistrm(currentSetBytes, currentStrBytes, 0));	// Find in set, return bit mask!
	if (!_mm_cmpistrs(currentSetBytes, currentStrBytes, 0))	// If properly optimized, will use result from previous _mm_cmpistrm call
		goto set_loop;
	if (_mm_cmpistrz(currentSetBytes, currentStrBytes, 0))	// If properly optimized, will use result from previous _mm_cmpistrm call
		return matchCounter + _mm_popcnt_u32(tmpMatches | tmpMatches2);

	sseSetIterator = (const __m128i *)set;	// Restore set pointer
	tmpMatches |= tmpMatches2;	// Accumulate matches
	goto set_finished;
}

size_t strCountInSetGeneric(const char *str, const char *set)
{
	auto setIt = set;
	size_t matchCounter = 0;	// Match counter

	while (auto currentStrChar = *str)	// Till end of string
	{
		while (auto currentSetChar = *setIt)	// Till end of set
		{
			++setIt;	// Next in set

			if (currentSetChar != currentStrChar)
				continue;

			++matchCounter;	// Count match
		}

		// End of set, no match found
		setIt = set;
		++str;	// Next in string
	}

	return matchCounter;
}

static size_t strCountInSetCPUDispath(const char *str, const char *set);
auto strCountInSetDispatch = strCountInSetCPUDispath;

static size_t strCountInSetCPUDispath(const char *str, const char *set)
{
	int instructionSet = InstructionSet();
	auto result = strCountInSetGeneric;

	if (instructionSet >= 9)
		result = strCountInSetSSE42;

	strCountInSetDispatch = result;

	return result(str, set);
}

extern "C" size_t strCountInSet(const char *str, const char *set)
{
	return strCountInSetDispatch(str, set);
}
