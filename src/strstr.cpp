#include "asmlib.h"
#include "asmlib-internal.h"
#include <immintrin.h>
#include <smmintrin.h>

template <typename T> T btr(T x, unsigned offset)
{
	return x & ~((T)1 << offset);
}

extern "C" char *strstrSSE42(char *haystack, const char *needle) __attribute__((target("sse4.2")));
extern "C" char *strstrSSE42(char *haystack, const char *needle)
{
	auto sseHaystack = (const __m128i *)haystack;
	auto sseNeedle = (const __m128i *)needle;
	auto firstNeedleBytes = _mm_loadu_si128(sseNeedle);
	__m128i mask;

	while (1)
	{
		auto alignedHaystackBytes = _mm_loadu_si128(sseHaystack);
		mask = _mm_cmpistrm(firstNeedleBytes, alignedHaystackBytes, 0b1100);	// Unsigned byte search, equal ordered, return mask
		if (_mm_cmpistrc(firstNeedleBytes, alignedHaystackBytes, 0b1100))	// Found beginning of a match
		{
			if (_mm_cmpistrz(firstNeedleBytes, alignedHaystackBytes, 0b1100))	// Haystack ends here, a short match is found
				return (char *)sseHaystack + asmlibInternal::bsf(_mm_cvtsi128_si32(mask));	// Match found within single paragraph, add index of first match

			auto convertedMask = _mm_cvtsi128_si32(mask);	// Bit mask of possible matches

			do
			{
				size_t indexOfPossibleMatch = asmlibInternal::bsf(convertedMask);	// Index of first bit in mask of possible matches

				auto sseHaystackIterator = (const __m128i *)((const char *)sseHaystack + indexOfPossibleMatch);	// Haystack + index
				auto sseNeedleIterator = sseNeedle;

				while (1)
				{
					// Compare loop for long match
					auto currentNeedleBytes = _mm_loadu_si128(sseNeedleIterator);	// Paragraph of needle
					auto currentHaystackBytes = _mm_loadu_si128(sseHaystackIterator);
					if (!_mm_cmpistro(currentNeedleBytes, currentHaystackBytes, 0b1100))	// Unsigned bytes, equal ordered
						break;	// Difference found after extending partial match
					if (_mm_cmpistrs(currentNeedleBytes, currentHaystackBytes, 0b1100))	// End of needle found, and no difference
						return (char *)sseHaystack + indexOfPossibleMatch;	// Haystack + index to begin of long match

					++sseNeedleIterator;
					++sseHaystackIterator;
				}

				// Remove index bit of first partial match
				convertedMask = btr(convertedMask, indexOfPossibleMatch);

			}
			while (convertedMask);	// Check whether mask contains more index bits, if so loop to next bit in mask

			// Mask exhausted for possible matches, continue to next haystack paragraph
			++sseHaystack;
			continue;	// Loop to next paragraph of haystack
		}
		if (_mm_cmpistrz(firstNeedleBytes, alignedHaystackBytes, 0b1100))	// End of haystack found, no match
			return 0;	// Needle not found, return 0

		++sseHaystack;
	}
}

extern "C" char *strstrGeneric(char *haystack, const char *needle)
{
	if (!needle[0])
		return haystack;	// A 0-length needle is always found

	if (!needle[1])
	{
		char needleChar = *needle;
		while (1)
		{
			auto currentChar = *haystack;
			if (!currentChar)
				return 0;	// End of haystack reached without finding

			if (needleChar == currentChar)
				return haystack;

			++haystack;
		}
	}

	auto currentNeedleCharacter = *needle;

	while (1)
	{
		// Search for first character match
		auto currentHaystackCharacter = *haystack;
		if (!currentHaystackCharacter)	// End of haystack reached without finding
			return 0;	// Needle not found, return 0

		if (currentNeedleCharacter == currentHaystackCharacter)
		{
			// First character match
			auto haystackIterator = haystack;	// Beginning of match position
			auto needleIterator = needle;

			do
			{
				++haystackIterator;
				++needleIterator;

				currentNeedleCharacter = *needleIterator;
				if (!currentNeedleCharacter)	// End of needle, match ok
					return haystack;	// Needle found, return pointer to position in haystack
			}
			while (currentNeedleCharacter == *haystackIterator);

			// Match failed, recover and continue
			currentNeedleCharacter = *needle;
		}

		++haystack;
	}
}

inline char *strstrCPUDispatch(char *haystack, const char *needle);
static auto strstrDispatch = strstrCPUDispatch;

inline char *strstrCPUDispatch(char *haystack, const char *needle)
{
	int instructionSet = InstructionSet();
	decltype(strstrDispatch) result = strstrGeneric;

	if (instructionSet >= asmlibInternal::InstructionSetReturnValues::sse42Supported)
		result = strstrSSE42;

	strstrDispatch = result;

	return result(haystack, needle);
}

extern "C" char *A_strstr(char *haystack, const char *needle)
{
	return strstrDispatch(haystack, needle);
}
