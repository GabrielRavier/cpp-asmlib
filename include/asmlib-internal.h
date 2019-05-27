#include "asmlib.h"
#include "asmlibran.h"
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
}
