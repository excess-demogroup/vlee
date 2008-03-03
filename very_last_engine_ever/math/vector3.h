#pragma once

namespace math
{
	class Vector3 : public D3DXVECTOR3
	{
	public:
		Vector3() {}
		Vector3(const D3DXVECTOR3 &v) : D3DXVECTOR3(v) {}
		Vector3(double x, double y, double z) : D3DXVECTOR3(float(x), float(y), float(z)) {}
	};
	
	inline float length(const Vector3 &v)
	{
		return sqrt(
			v.x * v.x +
			v.y * v.y +
			v.z * v.z
		);
	}
	
	inline Vector3 normalize(const Vector3 &v)
	{
		float len = length(v);
		float rcp = fabs(len) > 1e-5 ? 1.0f / len : 0.0f;
		return v * rcp;
	}
	
	inline float dot(const Vector3 &v1, const Vector3 &v2)
	{
		return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
	}
}
