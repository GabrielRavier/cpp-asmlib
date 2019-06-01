#include "asmlibran.h"
#include "asmlib-internal.h"
#include <cstring>

using namespace asmlibInternal::randomNumberGenerators;

inline uint32_t MotherInternalGenerate(mother::internals *pThis)
{
	if (pThis->intructionSet < asmlibInternal::InstructionSetReturnValues::sse2Supported)
	{
		// Generic version for old processors
		auto recallPreviousRandomNumber = pThis->m0;

		asmlibInternal::uint64Elements result;
		result.all = (uint64_t)pThis->factor3 * pThis->m3;	// x[n - 4]
		auto temporaryAccumulate = result;

		result.low = pThis->m2;	// x[n - 3]
		pThis->m3 = result.low;
		result.all = (uint64_t)result.low * pThis->factor2;
		temporaryAccumulate.all += result.all;

		result.low = pThis->m1;	// x[n - 2]
		pThis->m2 = result.low;
		result.all = (uint64_t)result.low * pThis->factor1;
		temporaryAccumulate.all += result.all;

		result.low = pThis->m0;	// x[n - 1]
		pThis->m1 = result.low;
		result.all = ((uint64_t)result.low * pThis->factor0) + temporaryAccumulate.all + (uint64_t)pThis->mCarry;

		// Store next number and carry
		pThis->m0 = result.low;
		pThis->mCarry = result.high;

		// Convert to float in case next call needs a float
		result.high = (result.low << 20);
		result.low = (result.low >> 12) | 0x3FF00000;

		asmlibInternal::doubleReinterpreter converter;
		converter.asInt = result;
		pThis->ranP1 = converter.asDouble;
		return recallPreviousRandomNumber;
	}
	else
	{
		// SSE2 version
		auto returnValue = pThis->m0;	// Retrieve previous random number
		auto m3m2m1m0 = _mm_load_si128((__m128i *)&pThis->m3);	// Load m3, m2, m1 and m0
		auto factors = _mm_load_si128((__m128i *)&pThis->factor3);	// Factors
		*(uint64_t *)&pThis->m4 = _mm_cvtsi128_si64(m3m2m1m0);	// m4 = m3, m3 = m2
		_mm_storeh_pi((__m64 *)&pThis->m2, (__m128)m3m2m1m0);	// m2 = m1, m1 = m0

		auto m2m0 = _mm_mul_epu32(_mm_srli_epi64(m3m2m1m0, 32), _mm_srli_epi64(factors, 32));	// m2 * mf2, m0 * mf0
		m3m2m1m0 = _mm_add_epi64(_mm_mul_epu32(m3m2m1m0, factors), m2m0);	// p2 + p3, p0 + p1
		auto completeSum = _mm_add_epi64(_mm_add_epi64(m3m2m1m0, (__m128i)_mm_movehl_ps((__m128)m2m0, (__m128)m3m2m1m0)), _mm_cvtsi64_si128(*(uint64_t *)&pThis->mCarry));	// p0 + p1 + p2 + p3 + carry
		*(uint64_t *)&pThis->m0 = _mm_cvtsi128_si64(completeSum);	// Store new m0 and carry

		// Convert to double precision float
		auto convertedToDouble = _mm_or_si128(_mm_srli_epi64(_mm_slli_epi64(completeSum, 32), 12), *(__m128i *)&pThis->one);	// Discard carry bits, get bits into mantissa position and add exponent bits to get number in interval [1, 2)

		asmlibInternal::doubleReinterpreter converter;

		converter.asInt.all = _mm_cvtsi128_si64(convertedToDouble);
		pThis->ranP1 = converter.asDouble;
		return returnValue;
	}
}

// Get random bits
extern "C" uint32_t MotBRandom(void *pThis)
{
	return MotherInternalGenerate((mother::internals *)(((uintptr_t)pThis) & -0x10));
}

// Get floating point random number
extern "C" double MotRandom(void *pThis)
{
	auto pInternals = (mother::internals *)((uintptr_t)pThis & -0x10);
	auto returnValue = pInternals->ranP1 - pInternals->one;

	MotherInternalGenerate(pInternals);

	return returnValue;
}

// Get random integer random number in desired interval
extern "C" int32_t MotIRandom(void *pThis, int32_t min, int32_t max)
{
	auto randomBits = MotherInternalGenerate((mother::internals *)((uintptr_t)pThis & -0x10));

	if (max < min)
		return 0x8000000;

	asmlibInternal::uint64Elements result;
	result.all = (uint64_t)randomBits * (min - max + 1);
	return result.high + max;
}

// Initialization
extern "C" void MotRandomInit(void *pThis, int seed)
{
	auto pInternals = (mother::internals *)((uintptr_t)pThis & -0x10);

	memset((uint8_t *)pInternals + 0x10, 0, sizeof(*pInternals) - 0x10);

	*(uint32_t *)((uint8_t *)&pInternals->one + 4) = 0x3FF00000;	// High dword of 1.0
	pInternals->factor0 = 5115;
	pInternals->factor1 = 1776;
	pInternals->factor2 = 1492;
	pInternals->factor3 = 2111111111UL;

	pInternals->intructionSet = InstructionSet();

	// Initialize from seed
	// Make random numbers and put them into buffer
	seed = (seed * 29943829) - 1;
	pInternals->m0 = seed;

	seed = (seed * 29943829) - 1;
	pInternals->m1 = seed;

	seed = (seed * 29943829) - 1;
	pInternals->m2 = seed;

	seed = (seed * 29943829) - 1;
	pInternals->m3 = seed;

	seed = (seed * 29943829) - 1;
	pInternals->mCarry = seed;

	// Randomize some more
	for (size_t i = 20; i; --i)
		MotherInternalGenerate(pInternals);
}

static mother::internals staticMotherInstance;

extern "C" void MotherRandomInit(int seed)
{
	MotRandomInit(&staticMotherInstance, seed);
}

extern "C" double MotherRandom()
{
	return MotRandom(&staticMotherInstance);
}

extern "C" int32_t MotherIRandom(int32_t min, int32_t max)
{
	return MotIRandom(&staticMotherInstance, min, max);
}

extern "C" uint32_t MotherBRandom()
{
	return MotBRandom(&staticMotherInstance);
}
