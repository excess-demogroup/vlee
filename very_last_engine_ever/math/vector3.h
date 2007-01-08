#pragma once

namespace math
{
	class Vector3 : public D3DXVECTOR3
	{
	public:
		Vector3(double x, double y, double z) : D3DXVECTOR3(float(x), float(y), float(z))
		{

		}

		void normalize()
		{
			float rcp = 1.f / (x * x + y * y + z * z);
			x *= rcp;
			y *= rcp;
			z *= rcp;
		}
	};

	inline float dot(const Vector3 &v1, const Vector3 &v2)
	{
		return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
	}
}
