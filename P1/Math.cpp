#include "Math.h"
#include <limits>
#include <cstring>


namespace Math
{
	bool compareReal(double a, double b)
	{
		return fabs(a - b) <= std::numeric_limits<double>::epsilon();
	}
	float radiansToDegrees(float radians)
	{
		return (radians / PI) * 180.0f;
	}
	float degreesToRadians(float degrees)
	{
		return degrees * (PI / 180.0f);
	}
}
