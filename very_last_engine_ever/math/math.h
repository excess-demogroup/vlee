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
		float t = clamp(x, 0.0f, 1.0f);
		return t * t * (3 - 2 * t);
	}

	template <typename T>
	inline T lerp(T v0, T v1, float t)
	{
		return v0 + (v1 - v0) * t;
	}

	inline float round(float f)
	{
		return floor(f + 0.5f);
	}
	
	inline float frac(float f)
	{
		return f - floor(f);
	}
}
