#pragma once

#include <algorithm>

namespace math
{
	inline float clamp(float x, float minval, float maxval)
	{
		return std::max(std::min(x, maxval), minval);
	}

	inline float smoothstep(float edge0, float edge1, float x)
	{
		x = (x - edge0) / (edge1 - edge0);
		float t = clamp(x, 0, 1);
		return t * t * (3 - 2 * t);
	}
}
