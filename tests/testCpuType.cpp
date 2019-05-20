#include "asmlib.h"
#include <iostream>

const char *vendorIDToStr(int vendor)
{
	switch (vendor)
	{
		case 1:
			return "Intel";

		case 2:
			return "AMD";

		case 3:
			return "VIA";

		case 4:
			return "Cyrix";

		case 5:
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
