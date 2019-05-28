#include "asmlib.h"
#include "asmlib-internal.h"
#include <cstddef>
#include <x86intrin.h>

static uint32_t getByteIndex(const __m128i *sseStr)
{
	// Read 16 bytes from aligned pointer, then compare them with 16 0s, then get the compare result into a 32-bit integer (1 bit for each byte)
	return _mm_movemask_epi8(_mm_cmpeq_epi8(_mm_load_si128(sseStr), _mm_setzero_si128()));
}

extern "C" size_t strlenSSE2(const char *str)
{
	const __m128i *sseStr = (const __m128i *)str;	// Get pointer to string
	uintptr_t misalignment = (uintptr_t)str;	// Copy pointer

	misalignment &= 0xF;	// Lower 4 bits indicate misalignment
	sseStr = (const __m128i *)((uintptr_t)sseStr & -0x10);	// Align pointer by 16

	uint32_t byteIndex = asmlibInternal::bsf((getByteIndex(sseStr) >> misalignment) << misalignment);	// Find first 1-bit, shifting out bad bits to avoid falsely reading random stuff as end of string

	while (!byteIndex)
	{
		++sseStr;	// Increment pointer by 16
		byteIndex = asmlibInternal::bsf(getByteIndex(sseStr));	// Find first 1-bit
	}

	// Zero-byte found, compute string length
	return ((const char *)sseStr - str) + byteIndex;	// Add byte index
}

extern "C" size_t strlen386(const char *str)
{
	uint32_t *u32Str = (uint32_t *)str;
	uint32_t mask = 0;	// Make sure we don't use mask uninitialized
	uintptr_t alignedPtr = (uintptr_t)str & 3;	// Get aligned pointer to string
	if (alignedPtr)	// Lower two bits of address, check alignment
	{
		// String is not aligned by 4, check unaligned bytes
		u32Str = (uint32_t *)((uintptr_t)u32Str & -4);	// Align pointer by 4

		uint32_t currentBytes;
		currentBytes = (*u32Str) | ~((uint32_t) -1 << (uint8_t)(alignedPtr << 3));	// Mask out false bytes

		// Check first four bytes for 0
		// currentBytes - 0x01010101 removes 1 from each byte
		// & 0x80808080 tests all sign bits
		mask = ((currentBytes - 0x01010101) & ~currentBytes) & 0x80808080;
	}
	else
		--u32Str;

	// Main loop, read 4 bytes aligned
	while (!mask)
	{
		++u32Str;
		uint32_t currentBytes = *u32Str;
		mask = ((currentBytes - 0x01010101) & ~currentBytes) & 0x80808080;
	}

	auto byteIndex = (uint32_t)asmlibInternal::bsf(mask) >> 3; // Find right-most 1-bits and divide by 8 : byte index
	return ((const char *)u32Str - str) + byteIndex;	// Subtract start address and add index
}

static size_t strlenCPUDispatch(const char *str);
auto strlenDispatch = strlenCPUDispatch;

static size_t strlenCPUDispatch(const char *str)
{
	auto instructionSet = InstructionSet();
	strlenDispatch = strlen386;

	if (instructionSet >= asmlibInternal::InstructionSetReturnValues::sse2Supported)
		strlenDispatch = strlenSSE2;

	return strlenDispatch(str);
}

extern "C" size_t A_strlen(const char *str)
{
#ifdef __i386__
	return strlenDispatch(str);
#else
	return strlenSSE2(str);
#endif
}
