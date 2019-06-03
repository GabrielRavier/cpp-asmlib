#include "asmlib.h"
#include "asmlibran.h"
#include <utility>
#include <climits>
#include <csignal>
#include <cpuid.h>
#include <immintrin.h>
#include <x86intrin.h>

/*
 *
 * This header contains internal stuff for asmlib
 *
 */

#define asmlib_likely(x) __builtin_expect((x),1)
#define asmlib_unlikely(x) __builtin_expect((x),0)

namespace asmlibInternal
{
	constexpr size_t maximumCacheLevel = 4;

	union uint64Elements
	{
		struct
		{
			uint32_t low;
			uint32_t high;
		};
		uint64_t all;
	};

	union int64Elements
	{
		struct
		{
			int32_t low;
			int32_t high;
		};
		int64_t all;
	};

	union doubleReinterpreter
	{
		uint64Elements asInt;
		double asDouble;
	};

	inline bool isCPUIDSupported()
	{
#if __i386__
		int cpuidSupported;

		__asm("  pushfl\n"
		"  popl   %%eax\n"
		"  movl   %%eax,%%ecx\n"
		"  xorl   $0x00200000,%%eax\n"
		"  pushl  %%eax\n"
		"  popfl\n"
		"  pushfl\n"
		"  popl   %%eax\n"
		"  movl   $0,%0\n"
		"  cmpl   %%eax,%%ecx\n"
		"  je     1f\n"
		"  cmpl   %%eax,%%ecx\n"
		"  movl   $1,%0\n"
		"1:"
		: "=r" (cpuidSupported) : : "eax", "ecx");
		if (!cpuidSupported)
			return false;
		return true;
#else
		// x86-64
		return true;
#endif
	}

	inline int32_t bsf(int32_t x)
	{
		return __builtin_ctz(x);
	}

	inline int32_t bsr(int32_t x)
	{
		return 31 - __builtin_clz(x);
	}

	[[noreturn]] inline void raiseIllegalInstructionError()
	{
#if defined(__i386__) || defined(__x86_64__)
		//__asm__ volatile("movl 1, %%edx; divl %%edx");	// Overflow error
		__asm__ volatile("ud2");	// Illegal instruction error
#endif

#if defined(__GNUC__)
		__builtin_trap();
#endif
		raise(SIGILL);

		for(;;)
			;
	}

	template <typename T> inline bool bitTest(T val, uint32_t offset)
	{
		auto mask = (T)(1) << (offset & ((sizeof(T) * CHAR_BIT) - 1));
		return (val & mask);
	}

	inline bool doesOSSupportSSE()
	{
		constexpr uint32_t testData = 0xD95A34BE;	// Random test value
		constexpr size_t testPosition = 0x10C;	// Position to write testData, upper part of xmm6 image

		// Check OS support for SSE
		char fxSaveBuffer[0x200] __attribute__((aligned(16)));	// Allocate 16-byte aligned buffer for fxsave

		_fxsave(fxSaveBuffer);	// Get FP/MMX and SSE registers
		uint32_t *testPosPtr = (uint32_t *)(fxSaveBuffer + testPosition);
		auto oldValue = *testPosPtr;	// Read part of xmm6
		*testPosPtr ^= testData;	// Change value

		_fxrstor(fxSaveBuffer);	// Load changed value into xmm6
		*testPosPtr = oldValue;	// Restore old value into buffer
		_fxsave(fxSaveBuffer);

		auto changedValue = *testPosPtr;	// Get changed xmm6 value
		*testPosPtr = oldValue;	// Restore old value into buffer
		_fxrstor(fxSaveBuffer);	// Load old value into xmm6

		oldValue ^= changedValue;	// Get difference between old and new value

		return oldValue == testData;	// Test if xmm6 was changed correctly
	}

	inline bool doesOSSupportAVX() __attribute__((target("avx")));	// Needs avx attribute for _xgetbv to work
	inline bool doesOSSupportAVX()
	{
		return (_xgetbv(0) & 6) == 6;
	}

	inline std::pair<uint32_t, uint32_t *> xstore(uint32_t *addr, uint32_t edxIn, uint32_t ecxIn)
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

	inline void *stosb(void *s, char c, size_t n)
	{
		void *ediOut;
		asm volatile (
			"rep stosb\n"
			: "=D" (ediOut)
			: "c" (n), "a" (c), "D" (s)
			: "memory");
		return ediOut;
	}

	namespace unalignedIsFasterReturnValues
	{
		constexpr int probablySlower = 0;	// Unaligned read is probably slower than alignment shift
		constexpr int unknown = 1;
		constexpr int probablyFaster = 2;	// Unaligned read is probably faster than alignment shift
	}

	namespace store256FasterReturnValues
	{
		constexpr int thirtyTwoBytesMemoryWriteSlowerOrAVXNotSupported = 0;
		constexpr int unknown = 1;
		constexpr int thirtyTwoBytesMemoryWriteFaster = 2;
	}

	namespace PhysicalSeedReturnValues
	{
		constexpr int failureOrNoSuitableInstructionAvailable = 0;
		constexpr int noPhysicalRNGUsedTSCInstead = 1;
		constexpr int usedVIAPhysicalRNG = 2;
		constexpr int usedIntelRNG = 3;
		constexpr int usedIntelSeedGenerator = 4;
	}

	namespace CpuTypeReturnValues
	{
		constexpr int Intel = 1;
		constexpr int AMD = 2;
		constexpr int VIA = 3;
		constexpr int Cyrix = 4;
		constexpr int NexGen = 5;
	}

	namespace InstructionSetReturnValues
	{
		constexpr int i386Only = 0;
		constexpr int mmxSupported = 1;
		constexpr int cmovAndFcomiSupported = 2;
		constexpr int sseSupported = 3;
		constexpr int sse2Supported = 4;
		constexpr int sse3Supported = 5;
		constexpr int ssse3Supported = 6;
		constexpr int sse41Supported = 8;
		constexpr int popcntSupported = 9;
		constexpr int sse42Supported = 10;
		constexpr int avxSupported = 11;
		constexpr int pclmulAndAESSupported = 12;
		constexpr int avx2Supported = 13;
		constexpr int fma3F16CBmi1Bmi2LzcntSupported = 14;
		constexpr int avx512FSupported = 15;
		constexpr int avx512BWAvx512DQAvx512VlSupported = 16;
	}

	namespace randomNumberGenerators
	{
		constexpr bool tempering = true;
		namespace mersenne
		{
#if 0
			// Define constants for MT11213A
			constexpr uint32_t N = 351;
			constexpr uint32_t M = 175;
			constexpr uint32_t R = 19;
			constexpr uint32_t A = 0xE4BD75F5;
			constexpr uint32_t U = 11;
			constexpr uint32_t S = 7;
			constexpr uint32_t T = 15;
			constexpr uint32_t L = 17;
			constexpr uint32_t B = 0x655E5280;
			constexpr uint32_t C = 0xFFD58000;
#else
			constexpr uint32_t N = 624;
			constexpr uint32_t M = 397;
			constexpr uint32_t R = 31;
			constexpr uint32_t A = 0x9908B0DF;
			constexpr uint32_t U = 11;
			constexpr uint32_t S = 7;
			constexpr uint32_t T = 15;
			constexpr uint32_t L = 18;
			constexpr uint32_t B = 0x9D2C5680;
			constexpr uint32_t C = 0xEFC60000;
#endif
			constexpr uint32_t lowerMaskConst = ((uint64_t)1 << R) - 1;	// Lower R bits
			constexpr uint32_t upperMaskConst = (uint32_t)((uint64_t)-1 << R);	// Upper 32 R bits

			struct internals
			{
				uint32_t alignmentFiller[4];
				uint32_t premadeInteger[4];	// Premade tempered integer numbers, ready to use
				double premadeFloat[5];	// Premade floating point numbers, ready to use (subtract 1.0)
				// Fifth element is for unaligned overrun if N % 4 == 1
				double temporaryFloat;	// Temporary storage of floating point random number
				uint32_t premadeIndex;	// Index to next premadeInteger and premadeFloat random number
				int instructionSet;
				uint32_t lastInterval;	// Last interval length for IRandomX
				uint32_t rejectionLimit;	// Rejection limit used by IRandomX
				uint32_t tmb[4];	// 4 copies of B constant
				uint32_t tmc[4];	// 4 copies of C constant
				double one[2];	// 2 copies of 1.0 constant
				uint32_t mtIndex;	// Index into MT buffer
				uint32_t upperMask;
				uint32_t lowerMask;
				uint32_t constA;
				uint32_t wrap1[4];	// MT buffer km wraparound
				uint32_t MT[N];	// MT history buffer (aligned by 16)
				uint32_t wrap2[4];	// MT buffer kk wraparound
				uint32_t padding[(N & 3 ? (4 - (N & 3)) : 0)];	// Align by 4
			} __attribute__((packed));
		}

		namespace mother
		{
			struct internals
			{
				uint32_t alignmentFiller[4];
				double one;
				int intructionSet;
				uint32_t m4;	// x[n - 4]
				uint32_t m3;	// x[n - 3] (aligned)
				uint32_t m2;	// x[n - 2]
				uint32_t m1;	// x[n - 1]
				uint32_t m0;	// x[n]
				uint32_t mCarry;	// Carry (aligned)
				uint32_t zero;	// Zero-extension of carry
				double ranP1;	// Double random number in interval [1, 2)
				uint32_t factor3;	// 2111111111 (aligned)
				uint32_t factor2;	// 1492
				uint32_t factor1;	// 1776
				uint32_t factor0;	// 5115
			} __attribute__((packed));

			constexpr uint32_t factor0Const = 5115;
			constexpr uint32_t factor1Const = 1776;
			constexpr uint32_t factor2Const = 1492;
			constexpr uint32_t factor3Const = 2111111111;
		}

		namespace sfmt
		{
			// Choose Mersenne exponent
			// Higher values give longer cycle length and use more memory
			constexpr uint32_t mersenneExponent = 11213;	// Other possibilities : 607, 1279, 2281, 4253, 19937, 44497

			// Size of state vector
			constexpr uint32_t getN(uint32_t mersenneExponent)
			{
				switch (mersenneExponent)
				{
					case 44497:
						return 348;

					case 19937:
						return 156;

					case 11213:
						return 88;

					case 4253:
						return 34;

					case 2281:
						return 18;

					case 1279:
						return 10;

					case 607:
						return 5;
				}
			}
			constexpr auto N = getN(mersenneExponent);

			// Position of intermediate feedback
			constexpr uint32_t getM(uint32_t mersenneExponent)
			{
				switch (mersenneExponent)
				{
					case 44497:
						return 330;

					case 19937:
						return 122;

					case 11213:
						return 68;

					case 4253:
						return 17;

					case 2281:
						return 12;

					case 1279:
						return 7;

					case 607:
						return 2;
				}
			}
			constexpr auto M = getM(mersenneExponent);

			// Left shift of W[N - 1], 32-bit words
			constexpr uint32_t getShiftLeft1(uint32_t mersenneExponent)
			{
				switch (mersenneExponent)
				{
					case 44497:
						return 5;

					case 19937:
						return 18;

					case 11213:
					case 1279:
						return 14;

					case 4253:
						return 20;

					case 2281:
						return 19;

					case 607:
						return 15;
				}
			}
			constexpr auto shiftLeft1 = getShiftLeft1(mersenneExponent);

			// Left shift of W[0], *8, 128-bit words
			constexpr uint32_t getShiftLeft2(uint32_t mersenneExponent)
			{
				switch (mersenneExponent)
				{
					case 44497:
					case 11213:
					case 1279:
					case 607:
						return 3;

					case 19937:
					case 4253:
					case 2281:
						return 1;
				}
			}
			constexpr auto shiftLeft2 = getShiftLeft2(mersenneExponent);

			// Right shift of W[M], 32-bit words
			constexpr uint32_t getShiftRight1(uint32_t mersenneExponent)
			{
				switch (mersenneExponent)
				{
					case 44497:
						return 9;

					case 19937:
						return 1;

					case 11213:
					case 4253:
						return 7;

					case 2281:
					case 1279:
						return 5;

					case 607:
						return 13;
				}
			}
			constexpr auto shiftRight1 = getShiftRight1(mersenneExponent);

			// Right shift of W[N - 2], *8, 128-bit words
			constexpr uint32_t getShiftRight2(uint32_t mersenneExponent)
			{
				switch (mersenneExponent)
				{
					case 44497:
					case 11213:
					case 607:
						return 3;

					case 19937:
					case 4253:
					case 2281:
					case 1279:
						return 1;
				}
			}
			constexpr auto shiftRight2 = getShiftRight2(mersenneExponent);

			// First dword of and mask
			constexpr auto getMask1(uint32_t mersenneExponent)
			{
				switch (mersenneExponent)
				{
					case 44497:
						return 0xEFFFFFFB;

					case 19937:
						return 0xDFFFFFEF;

					case 11213:
						return 0xEFFFF7FB;

					case 4253:
						return 0x9F7BFFFF;

					case 2281:
						return 0xBFF7FFBF;

					case 1279:
						return 0xF7FEFFFD;

					case 607:
						return 0xFDFF37FF;
				}
			}
			constexpr auto mask1 = getMask1(mersenneExponent);

			// Add SFMT_MASK when needed
			// Add SFMT_PARITY when needed
			// Add period certification vector when needed

			struct internals
			{
				uint32_t alignmentFiller[4];

				// Parameters for Mother-Of-All generator
				uint32_t m3;	// x[n - 3] (aligned)
				uint32_t unusedFillerToFitPmuludqInstruction1;

				uint32_t m2;	// x[n - 2]
				uint32_t unusedFillerToFitPmuludqInstruction2;

				uint32_t m1;	// x[n - 1]
				uint32_t unusedFillerToFitPmuludqInstruction3;

				uint32_t m0;	// x[n]
				uint32_t mCarry;	// Zero-extends into one

				double one;	// 1.0 (low dword : zero-extension of carry) (aligned)
				double temporaryRandomNumber;

				uint32_t filler3;	// 2111111111 (aligned)
				uint32_t instructionSet;

				uint32_t filler2;	// 1492
				uint32_t unusedFillerToFitPmuludqInstruction4;

				uint32_t filler1;	// 1776
				uint32_t unusedFillerToFitPmuludqInstruction5;

				uint32_t filler0;	// 5115
				uint32_t unusedFillerToFitPmuludqInstruction6;

				uint32_t lastIntervalLength;	// For IRandomX
				uint32_t rejectionLimit;	// For IRandomX

				uint32_t useMother;	// 1 if combine with Mother-Of-All generator
				uint32_t stateBufferIndex;	// Index into state buffer for SFMT

				uint32_t andMask[4];	// (aligned)
				uint32_t state[N + 4];	// (aligned)
			} __attribute__((packed));
		}
	}
}

// Function declarations for internal functions
extern "C"
{
	uint32_t popcountSSE42(uint32_t x);
	uint32_t popcountGeneric(uint32_t x);
	int strcmpGeneric(const char *str1, const char *str2);
	int strcmpSSE42(const char *str1, const char *str2);
	size_t strCountInSetGeneric(const char *str, const char *set);
	size_t strCountInSetSSE42(const char *str, const char *set);
	size_t strcount_UTF8SSE42(const char *str);
	size_t strcount_UTF8Generic(const char *str);
	size_t strlen386(const char *str);
	size_t strlenSSE2(const char *str);
	char *strstrGeneric(char *haystack, const char *needle);
	char *strstrSSE42(char *haystack, const char *needle);
	void strtoupperGeneric(char *str);
	void strtolowerGeneric(char *str);
	void strtoupperSSE42(char *str);
	void strtolowerSSE42(char *str);
	size_t strspnGeneric(const char *string, const char *set);
	size_t strcspnGeneric(const char *string, const char *set);
	size_t strspnSSE42(const char *string, const char *set);
	size_t strcspnSSE42(const char *string, const char *set);
	__m128i dividefixedV4i32SSE2(const __m128i buf[2], __m128i x);
	__m128i dividefixedV4i32SSE41(const __m128i buf[2], __m128i x);
}
