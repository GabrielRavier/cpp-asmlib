#include "asmlib.h"
#include "asmlib-internal.h"
#include <utility>
#include <climits>
#include <x86intrin.h>

static inline std::pair<uint32_t, uint32_t *> xstore(uint32_t *addr, uint32_t edxIn, uint32_t ecxIn)
{
	uint32_t eaxOut;
	uint32_t *ediOut;

	asm("rep xstore"
	: "=m" (*addr), "=a" (eaxOut), "=D" (ediOut)
	: "d" (edxIn), "D" (addr), "c" (ecxIn)
	: "memory"
	);

	return {eaxOut, ediOut};
}

static inline void *stosb(void *s, char c, size_t n) {
	void *ediOut;
	asm volatile (
		"rep stosb\n"
		: "=D" (ediOut)
		: "c" (n), "a" (c), "D" (s)
		: "memory");
	return ediOut;
}

namespace PhysicalSeedReturnValues
{
	constexpr int failureOrNoSuitableInstructionAvailable = 0;
	constexpr int noPhysicalRNGUsedTSCInstead = 1;
	constexpr int usedVIAPhysicalRNG = 2;
	constexpr int usedIntelRNG = 3;
	constexpr int usedIntelSeedGenerator = 4;
}

constexpr size_t maxIntelRDTries = 20;

int PhysicalSeedRDSeed(int seeds[], int numSeeds) __attribute__((target("rdseed")));
int PhysicalSeedRDRand(int seeds[], int numSeeds) __attribute__((target("rdrnd")));

int PhysicalSeedRDSeed(int seeds[], int numSeeds)
{
	if (!numSeeds)
		return PhysicalSeedReturnValues::usedIntelSeedGenerator;

#ifdef __x86_64__

	// Do 64-bits at a time
	auto numSeedsBy2 = numSeeds >> 1;
	if (!numSeedsBy2)
		goto skip64BitLoop;

	size_t remainingTries64;
	remainingTries64 = maxIntelRDTries;
	while (1)
	{
		uint64_t randomNum64;
		if (!_rdseed64_step((unsigned long long *)&randomNum64))
		{
			// Failed, try again
			if (!--remainingTries64)
				return PhysicalSeedReturnValues::failureOrNoSuitableInstructionAvailable;
			continue;
		}

		*(uint64_t *)seeds = randomNum64;
		seeds += 2;
		if (!--numSeeds)
			break;

		remainingTries64 = maxIntelRDTries;
	}

skip64BitLoop:
	numSeeds &= 1;
	if (!numSeeds)
		return PhysicalSeedReturnValues::usedIntelSeedGenerator;

#endif

	auto remainingTries = maxIntelRDTries;
	while (1)
	{
		uint32_t randomNum;
		if (!_rdseed32_step(&randomNum))
		{
			// Failed, try again
			if (!--remainingTries)
				return PhysicalSeedReturnValues::failureOrNoSuitableInstructionAvailable;
			continue;
		}

		*seeds = randomNum;
		++seeds;
		if (!--numSeeds)
			break;

		remainingTries = maxIntelRDTries;
	}

	return PhysicalSeedReturnValues::usedIntelSeedGenerator;
}

int PhysicalSeedRDRand(int seeds[], int numSeeds)
{
	if (!numSeeds)
		return PhysicalSeedReturnValues::usedIntelRNG;

#ifdef __x86_64__

	// Do 64-bits at a time
	auto numSeedsBy2 = numSeeds >> 1;
	if (!numSeedsBy2)
		goto skip64BitLoop;

	size_t remainingTries64;
	remainingTries64 = maxIntelRDTries;
	while (1)
	{
		uint64_t randomNum64;
		if (!_rdrand64_step((unsigned long long *)&randomNum64))
		{
			// Failed, try again
			if (!--remainingTries64)
				return PhysicalSeedReturnValues::failureOrNoSuitableInstructionAvailable;
			continue;
		}

		*(uint64_t *)seeds = randomNum64;
		seeds += 2;
		if (!--numSeedsBy2)
			break;

		remainingTries64 = maxIntelRDTries;
	}

skip64BitLoop:
	numSeeds &= 1;
	if (!numSeeds)
		return PhysicalSeedReturnValues::usedIntelRNG;

#endif

	auto remainingTries = maxIntelRDTries;
	while (1)
	{
		uint32_t randomNum;
		if (!_rdrand32_step(&randomNum))
		{
			// Failed, try again
			if (!--remainingTries)
				return PhysicalSeedReturnValues::failureOrNoSuitableInstructionAvailable;
			continue;
		}

		*seeds = randomNum;
		++seeds;
		if (!--numSeeds)
			break;

		remainingTries = maxIntelRDTries;
	}

	return PhysicalSeedReturnValues::usedIntelRNG;
}

int PhysicalSeedVIA(int seeds[], int numSeeds)
{
	auto roundedNumSeeds = numSeeds & -2;	// Round down to nearest even
	if (roundedNumSeeds)
	{
		roundedNumSeeds <<= 2;	// Number of bytes (divisible by 8)
#ifdef TESTING
		seeds = stosb(seeds, 1, roundedNumSeeds);
#else
		auto eaxEdi = xstore((uint32_t *)seeds, 3, roundedNumSeeds);	// Quality factor is 3. Generate random numbers

		seeds = (int *)eaxEdi.second;
#endif
	}

	if (!(numSeeds & 1))
		return PhysicalSeedReturnValues::usedVIAPhysicalRNG;

	// numSeeds is odd. Make 4/8 bytes in temporary buffer and store 4 of the bytes
	int temporaryBuffer[2] __attribute__((aligned(8)));

#ifdef TESTING
	stosb(temporaryBuffer, 1, 4);
#else
	xstore((uint32_t *)temporaryBuffer, 3, 4);	// Will generate 4 or 8 bytes, depending on CPU
#endif

	*seeds = *temporaryBuffer;
	return PhysicalSeedReturnValues::usedVIAPhysicalRNG;
}

int PhysicalSeedRDTSC(int seeds[], int numSeeds)
{
	uint32_t eax, ebx, ecx, edx;
	__cpuid(0, eax, ebx, ecx, edx);	// Serialize
	uint64_t timeStampCounter = __rdtsc();	// Get time stamp counter

	if (!numSeeds)
		return PhysicalSeedReturnValues::noPhysicalRNGUsedTSCInstead;

	if (numSeeds < 0)
		return PhysicalSeedReturnValues::failureOrNoSuitableInstructionAvailable;

	*seeds = (uint32_t)timeStampCounter;	// Store time stamp counter as seeds[0]
	++seeds;

	if (!--numSeeds)
		return PhysicalSeedReturnValues::noPhysicalRNGUsedTSCInstead;

	*seeds = (uint32_t)(timeStampCounter >> 32);	// Store upper part of time stamp counter as seeds[1]
	++seeds;

	if (!--numSeeds)
		return PhysicalSeedReturnValues::noPhysicalRNGUsedTSCInstead;

	do
	{
		*seeds = 0;	// Store 0 for the rest
		++seeds;
	} while (--numSeeds);

	return PhysicalSeedReturnValues::noPhysicalRNGUsedTSCInstead;
}

int PhysicalSeedNone(int seeds[], int numSeeds)
{
	// No possible generation
	if (!numSeeds)
		return PhysicalSeedReturnValues::failureOrNoSuitableInstructionAvailable;

	do
	{
		*seeds = 0;
		++seeds;
	} while (--numSeeds);

	return PhysicalSeedReturnValues::failureOrNoSuitableInstructionAvailable;
}

static int PhysicalSeedCPUDispatch(int seeds[], int numSeeds);

auto PhysicalSeedDispatch = PhysicalSeedCPUDispatch;

static int PhysicalSeedCPUDispatch(int seeds[], int numSeeds)
{
	auto result = PhysicalSeedNone;
	if (asmlibInternal::isCPUIDSupported())
	{
		uint32_t eax, ebx, ecx, edx;
		__cpuid(0, eax, ebx, ecx, edx);	// Get number of CPUID functions

		if (eax)	// Function 1 supported
		{
			// Test if RDSEED supported
			// __cpuid(0, eax, ebx, ecx, edx);
			if (eax >= 7)	// RDSEED supported
			{
				__cpuid_count(7, 0, eax, ebx, ecx, edx);
				if (asmlibInternal::bitTest(ebx, 18))
				{
					result = PhysicalSeedRDSeed;
					goto end;
				}

				__cpuid(1, eax, ebx, ecx, edx);
				if (asmlibInternal::bitTest(ecx, 30))
				{
					result = PhysicalSeedRDRand;
					goto end;
				}

				// Test if VIA xstore instruction supported
				__cpuid(0xC0000000, eax, ebx, ecx, edx);

				// Check for Centaur Extended Feature Flags support
				if (eax >= 0xC0000001)
				{
					__cpuid(0xC0000001, eax, ebx, ecx, edx);
					if (asmlibInternal::bitTest(edx, 3))	// Is RNG enabled (and available)
					{
						result = PhysicalSeedVIA;
						goto end;
					}
				}

				// Test if RDTSC supported
				__cpuid(1, eax, ebx, ecx, edx);

				if (asmlibInternal::bitTest(edx, 4))
				{
					result = PhysicalSeedRDTSC;
					goto end;
				}

				result = PhysicalSeedNone;
				goto end;
			}
		}
	}

end:
	PhysicalSeedDispatch = result;
	return result(seeds, numSeeds);
}

extern "C" int PhysicalSeed(int seeds[], int numSeeds)
{
	return PhysicalSeedDispatch(seeds, numSeeds);
}
