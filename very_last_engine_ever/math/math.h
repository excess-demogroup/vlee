#pragma once

#include <algorithm>

namespace math
{
	template <typename T>
	inline float clamp(T x, T minval, T maxval)
	{
		return std::max(std::min(x, maxval), minval);
	}

	template <typename T>
	inline T smoothstep(T edge0, T edge1, float x)
	{
		x = (x - edge0) / (edge1 - edge0);
		T t = clamp(x, 0, 1);
		return t * t * (3 - 2 * t);
	}

	float round(float f)
	{
		return floor(f + 0.5f);
	}
}
