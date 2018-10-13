#include "Math.h"
#include <limits>
#include <cstring>


namespace Math
{
	bool compareReal(double a, double b)
	{
		return fabs(a - b) <= std::numeric_limits<double>::epsilon();
	}
}
