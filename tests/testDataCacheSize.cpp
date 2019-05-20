#include <iostream>
#include "asmlib.h"

int main()
{
	std::cout << "Size of highest level data cache : " << DataCacheSize(0) << '\n';
	for (int i = 1; i < 4; ++i)
		std::cout << "Size of L" << i << " data cache : " << DataCacheSize(i) << '\n';
	return 0;
}
