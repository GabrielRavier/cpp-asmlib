#include "asmlib.h"
#include <smmintrin.h>

// Range for lowercase
constexpr char lowerCaseRange[16] =
{
	'a', 'z',
	'a', 'z',
	'a', 'z',
	'a', 'z',
	'a', 'z',
	'a', 'z',
	'a', 'z',
	'a', 'z',
};

// Range for uppercase
constexpr char upperCaseRange[16] =
{
	'A', 'Z',
	'A', 'Z',
	'A', 'Z',
	'A', 'Z',
	'A', 'Z',
	'A', 'Z',
	'A', 'Z',
	'A', 'Z',
};

// Bit to change when changing case
constexpr char bitToChange[16] =
{
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20
};

static void strToGenericSSE42(char *string, __m128i range, __m128i bitToChange) __attribute((target("sse4.2")));
static void strToGenericSSE42(char *string, __m128i range, __m128i bitToChange)
{
	auto sseString = (__m128i *)string;

	while (1)
	{
		// Loop
		auto currentStringBytes = _mm_loadu_si128(sseString);	// Read 16 bytes from string
		auto compareMask = _mm_cmpistrm(range, currentStringBytes, 0b1000100);	// Find bytes in range A-Z/a-z, return mask
		if (_mm_cmpistrz(range, currentStringBytes, 0b1000100))
		{
			// String ends in this paragraph

			// Write last 0-15 bytes
			// While we can read past the end of the string if precautions have been taken, we cannot write past the end of the string, even if the value is unchanged, because the value may have been changed in the meantime by another thread
			if (!_mm_cmpistrc(range, currentStringBytes, 0b1000100))
				return;

			currentStringBytes = _mm_xor_si128(currentStringBytes, _mm_and_si128(bitToChange, compareMask));	// Change case bit

#if 0

			// Method with maskmovdqu is elegant, but slow because maskmovdqu uses a nontemporal (uncached) write
			_mm_maskmoveu_si128(currentStringBytes, compareMask, (char *)sseString);

#else

			uint8_t *sseString8 = (uint8_t *)sseString;

			// Less elegant alternative, but probably faster if the data is needed again soon
			// Write 8-4-2-1 bytes, if necessary
			uint32_t eax = _mm_movemask_epi8(compareMask);	// Create bit mask
			if (eax < 0b10000000)
			{
				// There are less than 8 bytes to write
				*(uint64_t *)sseString8 = _mm_cvtsi128_si64(currentStringBytes);
				currentStringBytes = _mm_srli_si128(currentStringBytes, 8);
				sseString8 += 8;
				eax >>= 8;
			}

			if (eax >= 0b1000)
			{
				*(uint32_t *)sseString8 = _mm_cvtsi128_si32(currentStringBytes);
				currentStringBytes = _mm_srli_si128(currentStringBytes, 8);
				sseString8 += 4;
				eax >>= 4;
			}

			auto last4Bytes = _mm_cvtsi128_si32(currentStringBytes);
			if (eax >= 0b10)
			{
				*(uint16_t *)sseString8 = (uint16_t)last4Bytes;
				last4Bytes >>= 16;
				sseString8 += 2;
				eax >>= 2;
			}

			if (eax >= 1)
				*sseString8 = (uint8_t)last4Bytes;
#endif
			return;
		}

		// Change case bit in masked bytes of the string and write changed value
		_mm_storeu_si128(sseString, _mm_xor_si128(currentStringBytes, _mm_and_si128(compareMask, bitToChange)));
		++sseString;	// Next 16 bytes
	}

}

void strtolowerSSE42(char *string)
{
	strToGenericSSE42(string, _mm_load_si128((const __m128i *)upperCaseRange), _mm_load_si128((const __m128i *)bitToChange));
}

void strtoupperSSE42(char *string)
{
	strToGenericSSE42(string, _mm_load_si128((const __m128i *)lowerCaseRange), _mm_load_si128((const __m128i *)bitToChange));
}

static void strToGenericGeneric(char *string, char othercaseA, char othercaseZ, char convertedToCaseA)
{
	while (1)
	{
		uint8_t currentCharacter = *string;
		if (!currentCharacter)
			return;	// End of string

		currentCharacter -= othercaseA;
		if (currentCharacter <= (othercaseZ - othercaseA))
		{
			// Current character is other case, convert to converted-to case
			currentCharacter += convertedToCaseA;
			*string = currentCharacter;
		}

		++string;	// Next character
	}
}

void strtolowerGeneric(char *string)
{
	strToGenericGeneric(string, 'A', 'Z', 'a');
}

void strtoupperGeneric(char *string)
{
	strToGenericGeneric(string, 'a', 'z', 'A');
}

static void strtolowerCPUDispatch(char *string);
static void strtoupperCPUDispatch(char *string);

auto strtolowerDispatch = strtolowerCPUDispatch;
auto strtoupperDispatch = strtoupperCPUDispatch;

static void strtolowerCPUDispatch(char *string)
{
	auto result = strtolowerGeneric;

	if (InstructionSet() >= 10)
		result = strtolowerSSE42;

	strtolowerDispatch = result;

	result(string);
}

static void strtoupperCPUDispatch(char *string)
{
	auto result = strtoupperGeneric;

	if (InstructionSet() >= 10)
		result = strtoupperSSE42;

	strtoupperDispatch = result;

	result(string);
}

extern "C" void A_strtolower(char *string)
{
	strtolowerDispatch(string);
}

extern "C" void A_strtoupper(char *string)
{
	strtoupperDispatch(string);
}
