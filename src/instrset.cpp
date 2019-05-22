#include "asmlib.h"
#include <cpuid.h>
#include <climits>
#include <x86intrin.h>
#include <immintrin.h>

static bool isCPUIDSupported()
{
	#if __i386__
	int __cpuid_supported;

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
	"  movl   $1,%0\n"
	"1:"
	: "=r" (__cpuid_supported) : : "eax", "ecx");
	if (!__cpuid_supported)
		return false;
	return true;
	#else
	// x86-64
	return true;
	#endif
}

template <typename T> static bool bitTest(T val, uint32_t offset)
{
	auto mask = (T)(1) << (offset & ((sizeof(T) * CHAR_BIT) - 1));
	return (val & mask);
}

#ifdef __i386__

static bool doesOSSupportSSE()
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

#endif

static bool doesOSSupportAVX() __attribute__((target("avx")));	// Needs avx attribute for _xgetbv to work
static bool doesOSSupportAVX()
{
	uint64_t bv = _xgetbv(0);
	return (bv & 6) == 6;
}

namespace instructionSetRetVals
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

extern "C" int InstructionSet()
{
	static int supportedInstructionSet = -1;

	if (supportedInstructionSet >= 0)	// Negative means first time
		return supportedInstructionSet;	// Has been called before. Early return

	// Function has not been called before
	int result = instructionSetRetVals::i386Only;
	uint32_t eax, ebx, ecx, edx;

	if (!isCPUIDSupported())
		goto end;

#ifdef __i386__

	__cpuid(0, eax, ebx, ecx, edx);	// Get number of CPUID functions
	if (eax == 0)	// Function 1 not supported
		goto end;

#endif

	__cpuid(1, eax, ebx, ecx, edx);	// Get features

#ifdef __i386__

	// Check for floating point support and MMX support
	if (!bitTest(edx, 0) || !bitTest(edx, 23))
		goto end;

	result = instructionSetRetVals::mmxSupported;

	// Check for conditional move support
	if (!bitTest(edx, 15))
		goto end;

	result = instructionSetRetVals::cmovAndFcomiSupported;

	// Check CPU support for FXSAVE, OS support for SSE and CPU support for SSE
	if (!bitTest(edx, 24) || !doesOSSupportSSE() || !bitTest(edx, 25))
		goto end;

	result = instructionSetRetVals::sseSupported;

	// Check SSE2 support
	if (!bitTest(edx, 26))
		goto end;

#endif

	result = instructionSetRetVals::sse2Supported;

	// Check SSE3 support
	if (!bitTest(ecx, 0))
		goto end;

	result = instructionSetRetVals::sse3Supported;

	// Check SSSE3 support
	if (!bitTest(ecx, 9))
		goto end;

	result = instructionSetRetVals::ssse3Supported;

	// Check SSE4.1 support
	if (!bitTest(ecx, 19))
		goto end;

	result = instructionSetRetVals::sse41Supported;

	// Check popcnt support
	if (!bitTest(ecx, 23))
		goto end;

	result = instructionSetRetVals::popcntSupported;

	// Check SSE4.2 support
	if (!bitTest(ecx, 20))
		goto end;

	result = instructionSetRetVals::sse42Supported;

	// Check CPU support for xgetbv, OS support for AVX and CPU support for AVX
	if (!bitTest(ecx, 27) || doesOSSupportAVX() || !bitTest(ecx, 28))
		goto end;

	result = instructionSetRetVals::avxSupported;

	// Check pclmul and AES support
	if (!bitTest(ecx, 1) || !bitTest(ecx, 25))
		goto end;

	result = instructionSetRetVals::pclmulAndAESSupported;

	uint32_t savedEcx;
	savedEcx = ecx;	// Save ecx from original cpuid
	__cpuid_count(7, 0, eax, ebx, ecx, edx);

	// Check AVX2 support
	if (!bitTest(ebx, 5))
		goto end;

	ecx = savedEcx;
	result = instructionSetRetVals::avx2Supported;

	// Check FMA3, F16C, BMI1 and BMI2 support
	if (!bitTest(ecx, 12) || !bitTest(ecx, 29) || !bitTest(ebx, 3) || !bitTest(ebx, 8))
		goto end;

	// Check LZCNT support
	uint32_t savedEbx;
	savedEbx = ebx;
	savedEcx = ecx;
	__cpuid(0x80000001, eax, ebx, ecx, edx);
	if (!bitTest(ecx, 5))
		goto end;

	ebx = savedEbx;
	ecx = savedEcx;
	result = instructionSetRetVals::fma3F16CBmi1Bmi2LzcntSupported;

	// Check AVX512F support
	if (!bitTest(ebx, 16))
		goto end;

	result = instructionSetRetVals::avx512FSupported;

	// Check AVX512DQ, AVX512BW and AVX512VL support
	if (!bitTest(ebx, 17) || !bitTest(ebx, 30) || !bitTest(ebx, 31))
		goto end;

	result = instructionSetRetVals::avx512BWAvx512DQAvx512VlSupported;

end:
	supportedInstructionSet = result;
	return result;
}
