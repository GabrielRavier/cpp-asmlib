#include "asmlib.h"
#include "asmlib-internal.h"
#include <csignal>
#include <algorithm>

extern "C" void setdivisori32(int32_t buffer[2], int32_t d)
{
	int32_t shiftCount = -1;
	if (d - 1)
		shiftCount = asmlibInternal::bsr(d - 1);	// floor(log2(d-1))

	if (d < 0)
		asmlibInternal::raiseIllegalInstructionError();

	shiftCount = std::max(shiftCount, 0);	// Avoid negative shift count

	asmlibInternal::uint64Elements dividend;

	int32_t multiplier = 0;
	dividend.low = 0;
	dividend.high = 1;
	if (d != 1)	// Avoid division overflow when d == 1
	{
		dividend.high <<= shiftCount;
		multiplier = dividend.all / d;
	}

	++multiplier;

	buffer[0] = multiplier;
	buffer[1] = shiftCount;
}

extern "C" int32_t dividefixedi32(const int32_t buffer[2], int32_t x)
{
	asmlibInternal::int64Elements multResult;
	multResult.all = (int64_t)x * buffer[0];

	int32_t tmp = x + multResult.high;
	int32_t shiftCount = buffer[1];
	tmp >>= (uint8_t)shiftCount;

	// Subtract sign
	return (tmp - (-(x < 0)));
}

extern "C" void setdivisoru32(uint32_t buffer[2], uint32_t d)
{
	int32_t L = -1;
	if (d - 1)
		L = asmlibInternal::bsr(d - 1);	// L = floor(log2(d - 1))

	++L;	// L = ceil(log2(d))

	asmlibInternal::uint64Elements dividend;

	dividend.high = (uint64_t)1 << (uint8_t)L;	// 2^L (64 bit shift because L may be 32)
	dividend.high -= d;
	dividend.low = 0;

	buffer[0] = (dividend.all / d) + 1;	// Multiplier

	uint32_t shift1 = (L >= 1);	// Shift 1
	uint32_t shift2 = (L > 1 ? L - 1 : 0);	// Shift 2

	buffer[1] = (shift2 << 8) | shift1; // Shift 1 and shift 2
}

extern "C" uint32_t dividefixedu32(const uint32_t buffer[2], uint32_t x)
{
	asmlibInternal::uint64Elements multResult;
	multResult.all = (uint64_t)x * buffer[0];

	uint32_t shifts = buffer[1];
	x = (x - multResult.high) >> (uint8_t)shifts;	// Shift 1

	uint32_t result = x + multResult.high;
	shifts >>= 8;	// Shift 2
	return result >> (uint8_t)shifts;
}
