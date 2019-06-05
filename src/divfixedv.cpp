#include "asmlib.h"
#include "asmlib-internal.h"
#include <utility>

inline auto setdivisor8sInternal(int16_t d)
{
	int32_t bsrResult = -1;
	if (d - 1)
		bsrResult = asmlibInternal::bsr(d - 1);	// floor(log2(d - 1))

	if (d < 0)
		asmlibInternal::raiseIllegalInstructionError();

	if (bsrResult < 0)
		++bsrResult;	// Avoid negative shift count

	uint32_t divResult = 0;
	if (d != 1)
		divResult = ((1 << (uint8_t)bsrResult) << 16) / d;

	auto shiftCount = _mm_cvtsi32_si128(bsrResult);
	return std::make_pair(_mm_unpacklo_epi64(_mm_shufflelo_epi16(_mm_cvtsi32_si128(divResult + 1), 0), shiftCount), shiftCount);
}

extern "C" __m128i setdivisor8s(int16_t d)
{
	return setdivisor8sInternal(d).first;
}

extern "C" void setdivisorV8i16(__m128i buf[2], int16_t d)
{
	auto setDivisor8sResult = setdivisor8sInternal(d);
	buf[0] = _mm_unpacklo_epi64(setDivisor8sResult.first, setDivisor8sResult.first);	// Multiplier
	buf[1] = setDivisor8sResult.second;	// Shift count
}

extern "C" __m128i dividefixedV8i16(const __m128i buf[2], __m128i x)
{
	auto multiplied = _mm_mulhi_epi16(x, buf[0]);	// Multiply high signed words
	auto shifted = _mm_srai_epi16(_mm_add_epi16(multiplied, x), *(uint32_t *)&buf[1]);	// Shift right arithmetic
	auto sign = _mm_srai_epi16(x, 15);	// Sign of x
	return _mm_sub_epi16(shifted, sign);
}

// The stuff for u16 doesn't work properly
inline auto setdivisor8usInternal(uint16_t d)
{
	int32_t bsrResult = -1;
	if (d - 1)
		bsrResult = asmlibInternal::bsr(d - 1);	// floor(log2(d - 1))

	++bsrResult;	// ceil(log2(d))
	uint32_t multiplier = 1 << (uint8_t)bsrResult;	// Multiplier
	uint16_t divResult = (((multiplier - d) << 16) / d) + 1;
	auto sseMultiplier = _mm_shufflelo_epi16(_mm_cvtsi32_si128(divResult), 0); // Broadcast into lower 4 words
	auto sseShift1 = _mm_cvtsi32_si128(bsrResult >= 1);	// Shift 1
	auto sseShift2 = _mm_cvtsi32_si128((bsrResult - 1) & (uint8_t)-(bsrResult > 1));	// Shift 2
	auto sseShifts = _mm_unpacklo_epi32(sseShift1, sseShift2);	// Combine into 2 dwords
	return std::make_pair(_mm_unpacklo_epi64(sseMultiplier, sseShifts), sseShifts);
}

extern "C" __m128i setdivisor8us(uint16_t d)
{
	return setdivisor8usInternal(d).first;
}

extern "C" void setdivisorV8u16(__m128i buf[2], uint16_t d)
{
	auto setdivisor8usResult = setdivisor8usInternal(d);
	buf[0] = _mm_unpacklo_epi64(setdivisor8usResult.first, setdivisor8usResult.first);	// Multiplier
	buf[1] = setdivisor8usResult.second;	// Shift counts
}

extern "C" __m128i dividefixedV8u16(const __m128i buf[2], __m128i x)
{
	auto multiplied = _mm_mulhi_epu16(x, buf[0]);	// Multiply high unsigned words
	auto afterShift1 = _mm_srli_epi16(_mm_sub_epi16(x, multiplied), *((uint32_t *)&buf[1] + 0));	// Shift 1
	return _mm_srli_epi16(_mm_add_epi16(multiplied, afterShift1), *((uint32_t *)&buf[1] + 1));	// Shift 2
}

inline auto setdivisor4iInternal(int32_t d)
{
	int32_t bsrResult = -1;
	if (d - 1)
		bsrResult = asmlibInternal::bsr(d - 1);

	if (d < 0)
		asmlibInternal::raiseIllegalInstructionError();

	if (bsrResult < 0)
		++bsrResult;

	uint32_t divResult = 0;
	if (d != 1)
		divResult = (((uint64_t)1 << (uint8_t)bsrResult) << 32) / d;

	auto sseMultiplier = _mm_shuffle_epi32(_mm_cvtsi32_si128(divResult + 1), 0);
	auto shiftCount = _mm_cvtsi32_si128(bsrResult);
	return std::make_pair(_mm_unpacklo_epi64(sseMultiplier, shiftCount), shiftCount);
}

extern "C" __m128i setdivisor4i(int32_t d)
{
	return setdivisor4iInternal(d).first;
}

extern "C" void setdivisorV4i32(__m128i buf[2], int32_t d)
{
	auto setdivisor4iResult = setdivisor4iInternal(d);
	buf[0] = _mm_unpacklo_epi64(setdivisor4iResult.first, setdivisor4iResult.first);
	buf[1] = setdivisor4iResult.second;
}

extern "C" __m128i dividefixedV4i32SSE41(const __m128i buf[2], __m128i x) __attribute__((target("sse4.1")));
extern "C" __m128i dividefixedV4i32SSE41(const __m128i buf[2], __m128i x)
{
	auto multiplier = buf[0];
	auto multipliedX0X2 = _mm_mul_epi32(x, multiplier);	// 32 * 32 -> 64 bit signed multiplication of x[0] and x[2]
	auto multipliedX0X2HighDwords = _mm_srli_epi64(multipliedX0X2, 32);	// High dword of result 0 and 2
	auto positionedX1X3 = _mm_srli_epi64(x, 32);	// Get x[1] and x[3] into position for multiplication
	positionedX1X3 = _mm_mul_epi32(positionedX1X3, multiplier);	// 32 * 32 -> 64 bit signed multiplication of x[0] and x[2]
	auto multiplierMask = _mm_slli_epi64(_mm_cmpeq_epi32(multiplier, multiplier), 32);	// Generate mask of dword 1 and 3
	auto positionedX1X3HighDwords = _mm_and_si128(positionedX1X3, multiplierMask);	// High dword of result 1 and 3
	auto combinedResults = _mm_or_si128(multipliedX0X2HighDwords, positionedX1X3HighDwords);	// Combine all four results into one vector
	auto shiftedResults = _mm_srai_epi32(_mm_add_epi32(combinedResults, x), *(uint32_t *)&buf[1]);	// Shift right arithmetic
	auto signs = _mm_srai_epi32(x, 31);	// Sign of x
	return _mm_sub_epi32(shiftedResults, signs);
}

// Changing sign and using pmuludq doesn't work, gives rounding error
extern "C" __m128i dividefixedV4i32SSE2(const __m128i buf[2], __m128i x)
{
	union
	{
		int32_t xi32[4];
		__m128i xm128;
	} xConverter;

	int32_t multiplier = *(uint32_t *)&buf[0];
	xConverter.xm128 = x;

	// Do 4 signed high multiplications
	xConverter.xi32[0] = ((uint64_t)xConverter.xi32[0] * multiplier) >> 32;
	xConverter.xi32[1] = ((uint64_t)xConverter.xi32[1] * multiplier) >> 32;
	xConverter.xi32[2] = ((uint64_t)xConverter.xi32[2] * multiplier) >> 32;
	xConverter.xi32[3] = ((uint64_t)xConverter.xi32[3] * multiplier) >> 32;

	auto multiplied = xConverter.xm128;
	auto shifted = _mm_srai_epi32(_mm_add_epi32(multiplied, x), *(uint32_t *)&buf[1]);	// Shift right arithmetic
	auto signs = _mm_srai_epi32(x, 31);	// Sign of x
	return _mm_sub_epi32(shifted, signs);
}

inline __m128i dividefixedV4i32CPUDispatch(const __m128i buf[2], __m128i x);
static auto dividefixedV4i32Dispatch = dividefixedV4i32CPUDispatch;

inline __m128i dividefixedV4i32CPUDispatch(const __m128i buf[2], __m128i x)
{
	auto result = dividefixedV4i32SSE2;

	if (InstructionSet() >= asmlibInternal::InstructionSetReturnValues::sse41Supported)
		result = dividefixedV4i32SSE41;

	dividefixedV4i32Dispatch = result;

	return result(buf, x);
}

extern "C" __m128i dividefixedV4i32(const __m128i buf[2], __m128i x)
{
	return dividefixedV4i32Dispatch(buf, x);
}

inline auto setdivisor4uiInternal(uint32_t d)
{
	int32_t bsrResult = -1;
	if (d - 1)
		bsrResult = asmlibInternal::bsr(d - 1);	// floor(log2(d - 1))

	++bsrResult;	// ceil(log2(d))

	// 64-bit bit shift to allow overflow
	uint32_t divResult = (((((uint64_t)1 << (uint8_t)bsrResult) - d) << 32) / d) + 1;

	// Broadcast into 4 dwords
	auto multiplier = _mm_shuffle_epi32(_mm_cvtsi32_si128(divResult), 0);

	auto shift1 = _mm_cvtsi32_si128(bsrResult >= 1);
	auto shift2 = _mm_cvtsi32_si128((bsrResult - 1) & (uint8_t)-(bsrResult - 1));
	auto shifts = _mm_unpacklo_epi32(shift1, shift2);
	return std::make_pair(_mm_unpacklo_epi64(multiplier, shifts), shifts);
}

extern "C" __m128i setdivisor4ui(uint32_t d)
{
	return setdivisor4uiInternal(d).first;
}

extern "C" void setdivisorV4u32(__m128i buf[2], uint32_t d)
{
	auto setdivisor4uiResult = setdivisor4uiInternal(d);
	buf[0] = _mm_unpacklo_epi64(setdivisor4uiResult.first, setdivisor4uiResult.first);
	buf[1] = setdivisor4uiResult.second;
}

extern "C" __m128i dividefixedV4u32(const __m128i buf[2], __m128i x)
{
	auto multiplier = buf[0];
	auto multipliedX0X2 = _mm_mul_epu32(x, multiplier);	// 32 * 32 -> 64 unsigned multiplication of x[0] and x[2]
	auto multipliedX0X2HighDwords = _mm_srli_epi64(multipliedX0X2, 32);	// High dword of result 0 and 2
	auto positionedX1X3 = _mm_srli_epi64(x, 32);	// Get x[1] and x[3] into position for multiplication
	auto multipliedX1X3 = _mm_mul_epu32(positionedX1X3, multiplier);	// 32 * 32 -> 64 unsigned multiplication of x[1] and x[3]
	auto x1x3Mask = _mm_slli_epi64(_mm_cmpeq_epi32(multiplier, multiplier), 32);	// Generate mask of dword 1 and 3
	auto multipliedX1X3HighDwords = _mm_and_si128(multipliedX1X3, x1x3Mask);	// High dword of result 1 and 3
	auto combinedResults = _mm_or_si128(multipliedX0X2HighDwords, multipliedX1X3HighDwords);	// Combine all four results into one vector
	auto shiftedResults = _mm_srli_epi32(_mm_sub_epi32(x, combinedResults), *((uint32_t *)&buf[1] + 0));	// Shift 1
	return _mm_srli_epi32(_mm_add_epi32(combinedResults, shiftedResults), *((uint32_t *)&buf[1] + 1));	// Shift 1
}
