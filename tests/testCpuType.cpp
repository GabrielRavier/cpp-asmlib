#include "asmlib.h"
#include "asmlib-internal.h"
#include <iostream>

inline const char *vendorIDToStr(int vendor)
{
	switch (vendor)
	{
		case asmlibInternal::CpuTypeReturnValues::Intel:
			return "Intel";

		case asmlibInternal::CpuTypeReturnValues::AMD:
			return "AMD";

		case asmlibInternal::CpuTypeReturnValues::VIA:
			return "VIA";

		case asmlibInternal::CpuTypeReturnValues::Cyrix:
			return "Cyrix";

		case asmlibInternal::CpuTypeReturnValues::NexGen:
			return "NexGen";

		default:
			return "Unknown";
	}
}

int main()
{
	int vendor, family, model;
	CpuType(&vendor, &family, &model);

	std::cout << "Vendor is " << vendorIDToStr(vendor) << ", with ID of " << vendor << '\n'
			<< "Family is " << family << '\n'
			<< "Model is " << model << '\n';
}
