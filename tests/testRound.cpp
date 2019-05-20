#include "asmlib.h"
#include <iostream>

int main()
{
	for (double d = -2; d <= 2; d += 0.5)
		std::cout << "Round " << d << " = " << Round(d) << " = " << Round((float)d) << "\n";
}
