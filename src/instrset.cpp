#include "asmlib.h"
#include "asmlib-internal.h"
#include <climits>
#include <x86intrin.h>
#include <immintrin.h>

extern "C" int InstructionSet()
{
	static int supportedInstructionSet = -1;

	if (supportedInstructionSet >= 0)	// Negative means first time
		return supportedInstructionSet;	// Has been called before. Early return

	// Function has not been called before
	int result = asmlibInternal::InstructionSetReturnValues::i386Only;
	uint32_t eax, ebx, ecx, edx;

	if (!asmlibInternal::isCPUIDSupported())
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

	result = asmlibInternal::InstructionSetReturnValues::sse2Supported;

	// Check SSE3 support
	if (!asmlibInternal::bitTest(ecx, 0))
		goto end;

	result = asmlibInternal::InstructionSetReturnValues::sse3Supported;

	// Check SSSE3 support
	if (!asmlibInternal::bitTest(ecx, 9))
		goto end;

	result = asmlibInternal::InstructionSetReturnValues::ssse3Supported;

	// Check SSE4.1 support
	if (!asmlibInternal::bitTest(ecx, 19))
		goto end;

	result = asmlibInternal::InstructionSetReturnValues::sse41Supported;

	// Check popcnt support
	if (!asmlibInternal::bitTest(ecx, 23))
		goto end;

	result = asmlibInternal::InstructionSetReturnValues::popcntSupported;

	// Check SSE4.2 support
	if (!asmlibInternal::bitTest(ecx, 20))
		goto end;

	result = asmlibInternal::InstructionSetReturnValues::sse42Supported;

	// Check CPU support for xgetbv, OS support for AVX and CPU support for AVX
	if (!asmlibInternal::bitTest(ecx, 27) || asmlibInternal::doesOSSupportAVX() || !asmlibInternal::bitTest(ecx, 28))
		goto end;

	result = asmlibInternal::InstructionSetReturnValues::avxSupported;

	// Check pclmul and AES support
	if (!asmlibInternal::bitTest(ecx, 1) || !asmlibInternal::bitTest(ecx, 25))
		goto end;

	result = asmlibInternal::InstructionSetReturnValues::pclmulAndAESSupported;

	uint32_t savedEcx;
	savedEcx = ecx;	// Save ecx from original cpuid
	__cpuid_count(7, 0, eax, ebx, ecx, edx);

	// Check AVX2 support
	if (!asmlibInternal::bitTest(ebx, 5))
		goto end;

	ecx = savedEcx;
	result = asmlibInternal::InstructionSetReturnValues::avx2Supported;

	// Check FMA3, F16C, BMI1 and BMI2 support
	if (!asmlibInternal::bitTest(ecx, 12) || !asmlibInternal::bitTest(ecx, 29) || !asmlibInternal::bitTest(ebx, 3) || !asmlibInternal::bitTest(ebx, 8))
		goto end;

	// Check LZCNT support
	uint32_t savedEbx;
	savedEbx = ebx;
	savedEcx = ecx;
	__cpuid(0x80000001, eax, ebx, ecx, edx);
	if (!asmlibInternal::bitTest(ecx, 5))
		goto end;

	ebx = savedEbx;
	ecx = savedEcx;
	result = asmlibInternal::InstructionSetReturnValues::fma3F16CBmi1Bmi2LzcntSupported;

	// Check AVX512F support
	if (!asmlibInternal::bitTest(ebx, 16))
		goto end;

	result = asmlibInternal::InstructionSetReturnValues::avx512FSupported;

	// Check AVX512DQ, AVX512BW and AVX512VL support
	if (!asmlibInternal::bitTest(ebx, 17) || !asmlibInternal::bitTest(ebx, 30) || !asmlibInternal::bitTest(ebx, 31))
		goto end;

	result = asmlibInternal::InstructionSetReturnValues::avx512BWAvx512DQAvx512VLSupported;

end:
	supportedInstructionSet = result;
	return result;
}
