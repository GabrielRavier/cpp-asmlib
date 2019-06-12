#include "asmlib.h"
#include "asmlib-internal.h"
#include <iostream>
#include <string>

inline std::string getInstructionSetStr(int instrSet)
{
	std::string result = "All up to and including ";

	switch (instrSet)
	{
		case asmlibInternal::InstructionSetReturnValues::i386Only:
			return result + "i386";

		case asmlibInternal::InstructionSetReturnValues::mmxSupported:
			return result + "MMX";

		case asmlibInternal::InstructionSetReturnValues::cmovAndFcomiSupported:
			return result + "cmov and fcomi";

		case asmlibInternal::InstructionSetReturnValues::sseSupported:
			return result + "SSE";

		case asmlibInternal::InstructionSetReturnValues::sse2Supported:
			return result + "SSE2";

		case asmlibInternal::InstructionSetReturnValues::sse3Supported:
			return result + "SSE3";

		case asmlibInternal::InstructionSetReturnValues::ssse3Supported:
		case asmlibInternal::InstructionSetReturnValues::ssse3Supported + 1:
			return result + "SSSE3";

		case asmlibInternal::InstructionSetReturnValues::sse41Supported:
			return result + "SSE4.1";

		case asmlibInternal::InstructionSetReturnValues::popcntSupported:
			return result + "popcnt";

		case asmlibInternal::InstructionSetReturnValues::sse42Supported:
			return result + "SSE4.2";

		case asmlibInternal::InstructionSetReturnValues::avxSupported:
			return result + "AVX";

		case asmlibInternal::InstructionSetReturnValues::pclmulAndAESSupported:
			return result + "pclmul and AES";

		case asmlibInternal::InstructionSetReturnValues::avx2Supported:
			return result + "AVX2";

		case asmlibInternal::InstructionSetReturnValues::fma3F16CBmi1Bmi2LzcntSupported:
			return result + "FMA3, F16C, BMI1, BMI2 and lzcnt";

		case asmlibInternal::InstructionSetReturnValues::avx512FSupported:
			return result + "AVX512F";

		case asmlibInternal::InstructionSetReturnValues::avx512BWAvx512DQAvx512VLSupported:
			return result + "AVX512BW, AVX512DQ and AVX512VL";

		default:
			if (instrSet < asmlibInternal::InstructionSetReturnValues::i386Only)
				return "Function broken";

			if (instrSet > asmlibInternal::InstructionSetReturnValues::avx512BWAvx512DQAvx512VLSupported)
				return "Above AVX512BW, AVX512DQ and AVX512VL";
	}

	return "WTF";
}

int main()
{
	int instructionSet = InstructionSet();

	std::cout << "Reported instruction set : " << instructionSet << " (" << getInstructionSetStr(instructionSet) << ")\n";
}
