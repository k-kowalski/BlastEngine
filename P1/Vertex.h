#pragma once
#include "Math.h"

namespace Graphics
{
	struct Vertex
	{
		Math::Vector3 position;
		Math::Vector2 uv;
		Math::Vector3 normal;
	};
}