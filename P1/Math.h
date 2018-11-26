#pragma once
namespace Math
{
	const double PI = 3.14159265358979323846;

	struct Vector2
	{
		float x, y;
	};

	struct Vector3
	{
		float x, y, z;
	};

	struct Vector4
	{
		float x, y, z, w;
	};
	
	bool compareReal(double a, double b);
	float radiansToDegrees(float radians);
	float degreesToRadians(float radians);
};

