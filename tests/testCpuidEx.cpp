#include "asmlib.h"
#include <iostream>

static std::string getVendorString()
{
	int abcd[4];
	char vendorStr[13];
	cpuid_abcd(abcd, 0);
	*(int *)(vendorStr) = abcd[1];	// ebx
	*(int *)(vendorStr + 4) = abcd[3];	// edx
	*(int *)(vendorStr + 8) = abcd[2];	// ecx
	vendorStr[12] = '\0';	// Terminate string
	return vendorStr;
}

int main()
{
	std::cout << "Vendor string : \"" << getVendorString() << "\"\n";
}
