#include "asmlib.h"
#include "asmlib-internal.h"
inline int memcmpAVX512InternalDifferenceFound(__m512i currentPtr1Bytes, __m512i currentPtr2Bytes, __mmask16 bytesDiffMask) __attribute__((target("avx512f")));

inline int memcmpAVX512Internal(const void *ptr1, const void *ptr2, size_t size) __attribute__((target("avx512f")));

inline int memcmpAVX512BWInternalSize0To40(const void *ptr1, const void *ptr2, size_t size) __attribute__((target("avx512f"), target("avx512bw"), target("bmi2")));
inline int memcmpAVX512BWInternalSize0To40(const void *ptr1, const void *ptr2, size_t size)
{
	auto countPart1 = _cvtu32_mask32(_bzhi_u32(-1, size));
	if (size < 0x20)
		size = 0;
	else
		size -= 0x20;

	auto countPart2 = _cvtu32_mask32(_bzhi_u32(-1, size));
	auto count1s = _mm512_kunpackd(countPart2, countPart1);

	auto currentPtr1Bytes = _mm512_maskz_loadu_epi8(count1s, ptr1);
	auto currentPtr2Bytes = _mm512_maskz_loadu_epi8(count1s, ptr2);
	auto bytesDiffMask = _mm512_cmpneq_epi32_mask(currentPtr1Bytes, currentPtr2Bytes);

	if (!_mm512_kortestz(bytesDiffMask, bytesDiffMask))
		return memcmpAVX512InternalDifferenceFound(currentPtr1Bytes, currentPtr2Bytes, bytesDiffMask);

	return 0;
}

inline int memcmpAVX512BWInternalSize41To80(const void *ptr1, const void *ptr2, size_t size) __attribute__((target("avx512bw")));
inline int memcmpAVX512BWInternalSize41To80(const void *ptr1, const void *ptr2, size_t size)
{
	auto ptr18 = (const uint8_t *)ptr1;
	auto ptr28 = (const uint8_t *)ptr2;

	auto currentPtr1Bytes = _mm512_load_si512(ptr18);
	auto currentPtr2Bytes = _mm512_load_si512(ptr28);
	auto bytesDiffMask = _mm512_cmpneq_epi32_mask(currentPtr1Bytes, currentPtr2Bytes);

	if (!_mm512_kortestz(bytesDiffMask, bytesDiffMask))
		return memcmpAVX512InternalDifferenceFound(currentPtr1Bytes, currentPtr2Bytes, bytesDiffMask);

	return memcmpAVX512BWInternalSize0To40(ptr18 + 0x40, ptr28 + 0x40, size - 0x40);
}

extern "C" int memcmpAVX512BW(const void *ptr1, const void *ptr2, size_t size) __attribute__((target("avx512bw")));
extern "C" int memcmpAVX512BW(const void *ptr1, const void *ptr2, size_t size)
{
	if (size <= 0x40)
		return memcmpAVX512BWInternalSize0To40(ptr1, ptr2, size);
	else if (size <= 0x80)
		return memcmpAVX512BWInternalSize41To80(ptr1, ptr2, size);
	else
		return memcmpAVX512Internal(ptr1, ptr2, size);
}

inline int memcmpAVX512InternalDifferenceFound(__m512i currentPtr1Bytes, __m512i currentPtr2Bytes, __mmask16 bytesDiffMask)
{
	auto differentDwordPtr1 = _mm_cvtsi128_si32(_mm512_castsi512_si128(_mm512_maskz_compress_epi32(bytesDiffMask, currentPtr1Bytes)));
	auto differentDwordPtr2 = _mm_cvtsi128_si32(_mm512_castsi512_si128(_mm512_maskz_compress_epi32(bytesDiffMask, currentPtr2Bytes)));

	auto difference = differentDwordPtr1 ^ differentDwordPtr2;	// Difference
	auto differencePosition = asmlibInternal::bsf(difference);	// Position of lowest differing bit
	differencePosition &= -8;	// Round down to byte boundary
	differentDwordPtr1 <<= (uint8_t)differencePosition;	// First differing byte in al
	differentDwordPtr2 <<= (uint8_t)differencePosition;	// First differing byte in dl
	return differentDwordPtr1 - differentDwordPtr2;	// Signed difference between unsigned bytes
}

inline int memcmpAVX512Internal(const void *ptr1, const void *ptr2, size_t size)
{
	// Size >= 0x80
	auto ptr18 = (const uint8_t *)ptr1;
	auto ptr28 = (const uint8_t *)ptr2;
	auto currentPtr1Bytes = _mm512_loadu_si512((const __m512i *)ptr18);
	auto currentPtr2Bytes = _mm512_loadu_si512((const __m512i *)ptr28);
	auto bytesDiffMask = _mm512_cmpneq_epi32_mask(currentPtr1Bytes, currentPtr2Bytes);	// Compare first 0x40 bytes for dwords not equal

	if (!_mm512_kortestz(bytesDiffMask, bytesDiffMask))
		return memcmpAVX512InternalDifferenceFound(currentPtr1Bytes, currentPtr2Bytes, bytesDiffMask);

	// Find 0x40 boundaries
	auto endPtr1 = (const uint8_t *)ptr18 + size;
	auto alignedPtr1 = (const uint8_t *)((uintptr_t)(ptr18 + 1) & -0x40);	// First aligned boundary for ptr1
	ptr28 -= (ptr18 - alignedPtr1);	// Same offset to edi
	auto lastAlignedPtr1 = (const uint8_t *)((uintptr_t)endPtr1 & -0x40);	// Last aligned boundary for ptr1
	auto minSizeAlignedBlocks = alignedPtr1 - lastAlignedPtr1;	// -size of aligned blocks
	ptr28 -= minSizeAlignedBlocks;

	do
	{
		// Main loop
		currentPtr1Bytes = _mm512_load_epi64((const __m512i *)(lastAlignedPtr1 + minSizeAlignedBlocks));
		currentPtr2Bytes = _mm512_load_epi64((const __m512i *)(ptr28 + minSizeAlignedBlocks));
		bytesDiffMask = _mm512_cmpneq_epi32_mask(currentPtr1Bytes, currentPtr2Bytes);	// Compare first 0x40 bytes for dwords not equal

		if (!_mm512_kortestz(bytesDiffMask, bytesDiffMask))
			return memcmpAVX512InternalDifferenceFound(currentPtr1Bytes, currentPtr2Bytes, bytesDiffMask);

		minSizeAlignedBlocks += 0x40;
	}
	while (minSizeAlignedBlocks);

	// Remaining 0-0x3F bytes. Overlap with previous block
	ptr28 += endPtr1 - lastAlignedPtr1;

	currentPtr1Bytes = _mm512_loadu_si512((const __m512i *)(endPtr1 - 0x40));
	currentPtr2Bytes = _mm512_loadu_si512((const __m512i *)(ptr28 - 0x40));
	bytesDiffMask = _mm512_cmpneq_epi32_mask(currentPtr1Bytes, currentPtr2Bytes);	// Compare first 0x40 bytes for dwords not equal
	if (!_mm512_kortestz(bytesDiffMask, bytesDiffMask))
		return memcmpAVX512InternalDifferenceFound(currentPtr1Bytes, currentPtr2Bytes, bytesDiffMask);

	return 0;
}

extern "C" int memcmpAVX2(const void *ptr1, const void *ptr2, size_t size) __attribute__((target("avx2")));

extern "C" int memcmpAVX512F(const void *ptr1, const void *ptr2, size_t size) __attribute__((target("avx512f")));
extern "C" int memcmpAVX512F(const void *ptr1, const void *ptr2, size_t size)
{
	if (size >= 0x80)
		return memcmpAVX512Internal(ptr1, ptr2, size);

	return memcmpAVX2(ptr1, ptr2, size);
}


extern "C" int memcmpAVX2(const void *ptr1, const void *ptr2, size_t size)
{
	auto ptr18 = (const uint8_t *)ptr1;
	auto ptr28 = (const uint8_t *)ptr2;
	ssize_t ssize = size;
	ptr18 += ssize;	// Use negative index from end of memory block
	ptr28 += ssize;

	ssize = -ssize;
	if (!ssize)
		return 0;

	while (ssize <= -0x20)
	{
		auto ymmMask = _mm256_cmpeq_epi8(_mm256_loadu_si256((const __m256i *)(ptr18 + ssize)), *(const __m256i *)(ptr28 + ssize));	// Compare 32 bytes
		auto byteMask = _mm256_movemask_epi8(ymmMask);	// Get byte mask
		byteMask = ~byteMask;
		if (byteMask)
		{
			// Difference found, find position
			ssize += asmlibInternal::bsf(byteMask);
			return ptr18[ssize] - ptr28[ssize];
		}

		ssize += 32;
		if (!ssize)
			return 0;
	}

	if (ssize <= -0x10)
	{
		auto xmmMask = _mm_cmpeq_epi8(_mm_loadu_si128((const __m128i *)(ptr18 + ssize)), _mm_loadu_si128((const __m128i *)(ptr28 + ssize)));	// Compare 16 bytes
		auto byteMask = _mm_movemask_epi8(xmmMask);	// Get byte mask
		byteMask ^= 0xFFFF;	// Not ax
		if (byteMask)
		{
			// Difference found, find position
			ssize += asmlibInternal::bsf(byteMask);
			return ptr18[ssize] - ptr28[ssize];
		}

		ssize += 16;
		if (!ssize)
			return 0;
	}

	// Less than 16 bytes left
	if (ssize <= -8)
	{
		auto xmmMask = _mm_cmpeq_epi8(_mm_cvtsi64_si128(*(const uint64_t *)(ptr18 + ssize)), _mm_cvtsi64_si128(*(const uint64_t *)(ptr28 + ssize)));	// Compare 8 bytes
		auto byteMask = _mm_movemask_epi8(xmmMask);	// Get byte mask
		byteMask ^= 0xFFFF;	// Not ax
		if (byteMask)
		{
			// Difference found, find position
			ssize += asmlibInternal::bsf(byteMask);
			return ptr18[ssize] - ptr28[ssize];
		}

		ssize += 8;
		if (!ssize)
			return 0;
	}

	// Less than 8 bytes left
	if (ssize <= -4)
	{
		auto xmmMask = _mm_cmpeq_epi8(_mm_cvtsi32_si128(*(const uint32_t *)(ptr18 + ssize)), _mm_cvtsi32_si128(*(const uint32_t *)(ptr28 + ssize)));	// Compare 4 bytes
		auto byteMask = _mm_movemask_epi8(xmmMask);	// Get byte mask
		byteMask ^= 0xFFFF;	// Not ax
		if (byteMask)
		{
			// Difference found, find position
			ssize += asmlibInternal::bsf(byteMask);
			return ptr18[ssize] - ptr28[ssize];
		}

		ssize += 4;
		if (!ssize)
			return 0;
	}

	// Less than 4 bytes left
	if (ssize <= -2)
	{
		uint16_t ax = *(const uint16_t *)(ptr18 + ssize) - *(const uint16_t *)(ptr28 + ssize);
		if (!ax)
		{
			// Difference in byte 0 or 1
			if (!(uint8_t)ax)
				++ssize;

			return ptr18[ssize] - ptr28[ssize];
		}

		ssize += 2;
		if (!ssize)
			return 0;
	}

	// Less than 2 bytes left
	if (!ssize)
		return ptr18[ssize] - ptr28[ssize];

	return 0;
}

extern "C" int memcmpSSE2(const void *ptr1, const void *ptr2, size_t size)
{
	auto ptr18 = (const uint8_t *)ptr1;
	auto ptr28 = (const uint8_t *)ptr2;
	ssize_t ssize = size;
	ptr18 += ssize;	// Use negative index from end of memory block
	ptr28 += ssize;

	ssize = -ssize;
	if (!ssize)
		return 0;

	while (ssize <= -0x10)
	{
		auto xmmMask = _mm_cmpeq_epi8(_mm_loadu_si128((const __m128i *)(ptr18 + ssize)), _mm_loadu_si128((const __m128i *)(ptr28 + ssize)));	// Compare 16 bytes
		auto byteMask = _mm_movemask_epi8(xmmMask);	// Get byte mask
		byteMask ^= 0xFFFF;	// Not ax
		if (byteMask)
		{
			// Difference found, find position
			ssize += asmlibInternal::bsf(byteMask);
			return ptr18[ssize] - ptr28[ssize];
		}

		ssize += 16;
		if (!ssize)
			return 0;
	}

	// Less than 16 bytes left
	if (ssize <= -8)
	{
		auto xmmMask = _mm_cmpeq_epi8(_mm_cvtsi64_si128(*(const uint64_t *)(ptr18 + ssize)), _mm_cvtsi64_si128(*(const uint64_t *)(ptr28 + ssize)));	// Compare 8 bytes
		auto byteMask = _mm_movemask_epi8(xmmMask);	// Get byte mask
		byteMask ^= 0xFFFF;	// Not ax
		if (byteMask)
		{
			// Difference found, find position
			ssize += asmlibInternal::bsf(byteMask);
			return ptr18[ssize] - ptr28[ssize];
		}

		ssize += 8;
		if (!ssize)
			return 0;
	}

	// Less than 8 bytes left
	if (ssize <= -4)
	{
		auto xmmMask = _mm_cmpeq_epi8(_mm_cvtsi32_si128(*(const uint32_t *)(ptr18 + ssize)), _mm_cvtsi32_si128(*(const uint32_t *)(ptr28 + ssize)));	// Compare 4 bytes
		auto byteMask = _mm_movemask_epi8(xmmMask);	// Get byte mask
		byteMask ^= 0xFFFF;	// Not ax
		if (byteMask)
		{
			// Difference found, find position
			ssize += asmlibInternal::bsf(byteMask);
			return ptr18[ssize] - ptr28[ssize];
		}

		ssize += 4;
		if (!ssize)
			return 0;
	}

	// Less than 4 bytes left
	if (ssize <= -2)
	{
		uint16_t ax = *(const uint16_t *)(ptr18 + ssize) - *(const uint16_t *)(ptr28 + ssize);
		if (!ax)
		{
			// Difference in byte 0 or 1
			if (!(uint8_t)ax)
				++ssize;

			return ptr18[ssize] - ptr28[ssize];
		}

		ssize += 2;
		if (!ssize)
			return 0;
	}

	// Less than 2 bytes left
	if (!ssize)
		return ptr18[ssize] - ptr28[ssize];

	return 0;
}

extern "C" int memcmp386(const void *ptr1, const void *ptr2, size_t size)
{
	// Doesn't use rep cmps instructions but basically equivalent
	auto ptr132 = (const uint32_t *)ptr1;
	auto ptr232 = (const uint32_t *)ptr2;
	auto currentSize = size >> 2;
	bool endLoop = !currentSize;

	do
	{
		if (!currentSize--)
			break;

		endLoop = *ptr132++ == *ptr232++;
	} while (endLoop);

	if (endLoop)
	{
		currentSize = size & 3;
		endLoop = !currentSize;
	}
	else
	{
		currentSize = 4;
		--ptr132;
		--ptr232;
		endLoop = !ptr232;
	}

	auto ptr18 = (const uint8_t *)ptr132;
	auto ptr28 = (const uint8_t *)ptr232;
	do
	{
		if (!currentSize--)
			break;
		endLoop = *ptr18++ == *ptr28++;
	} while (endLoop);

	if (endLoop)
		return 0;
	else
		return *(ptr18 - 1) - *(ptr28 - 1);
}

inline int memcmpCPUDispatch(const void *ptr1, const void *ptr2, size_t size);
static auto memcmpDispatch = memcmpCPUDispatch;

inline int memcmpCPUDispatch(const void *ptr1, const void *ptr2, size_t size)
{
	auto result = memcmp386;
	auto instructionSet = InstructionSet();

	if (instructionSet >= asmlibInternal::InstructionSetReturnValues::sse2Supported)
	{
		if (instructionSet >= asmlibInternal::InstructionSetReturnValues::avx2Supported)
		{
			if (instructionSet >= asmlibInternal::InstructionSetReturnValues::avx512FSupported)
			{
				if (instructionSet >= asmlibInternal::InstructionSetReturnValues::avx512BWAvx512DQAvx512VLSupported)
					result = memcmpAVX512BW;
				else
					result = memcmpAVX512F;
			}
			else
				result = memcmpAVX2;
		}
		else
			result = memcmpSSE2;
	}

	memcmpDispatch = result;

	return result(ptr1, ptr2, size);
}

int A_memcmp(const void *ptr1, const void *ptr2, size_t size)
{
	return memcmpDispatch(ptr1, ptr2, size);
}
