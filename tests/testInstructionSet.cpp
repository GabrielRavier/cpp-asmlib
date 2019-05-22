#include "asmlib.h"
#include <iostream>
#include <string>

static std::string getInstructionSetStr(int instrSet)
{
	std::string result = "All up to and including ";

	switch (instrSet)
	{
		case 0:
			return result + "i386";

		case 1:
			return result + "MMX";

		case 2:
			return result + "cmov and fcomi";

		case 3:
			return result + "SSE";

		case 4:
			return result + "SSE2";

		case 5:
			return result + "SSE3";

		case 6:
		case 7:
			return result + "SSSE3";

		case 8:
			return result + "SSE4.1";

		case 9:
			return result + "popcnt";

		case 10:
			return result + "SSE4.2";

		case 11:
			return result + "AVX";

		case 12:
			return result + "pclmul and AES";

		case 13:
			return result + "AVX2";

		case 14:
			return result + "FMA3, F16C, BMI1, BMI2 and lzcnt";

		case 15:
			return result + "AVX512F";

		case 16:
			return result + "AVX512BW, AVX512DQ and AVX512VL";

		default:
			if (instrSet < 0)
				return "Function broken";

			if (instrSet > 16)
				return "Above AVX512BW, AVX512DQ and AVX512VL";
	}

	return "WTF";
}

int main()
{
	int instrSet = InstructionSet();

	std::cout << "Reported instruction set : " << instrSet << " (" << getInstructionSetStr(instrSet) << ")\n";

}
