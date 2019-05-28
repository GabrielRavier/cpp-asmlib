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

namespace asmlibInternal
{
	constexpr size_t maximumCacheLevel = 4;

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
}
