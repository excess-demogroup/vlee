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
	
	inline Vector3 cross(const Vector3 &v1, const Vector3 &v2)
	{
		return Vector3(v1.y*v2.z - v1.z*v2.y, v1.z*v2.x - v1.x*v2.z, v1.x*v2.y - v1.y*v2.x);
	}

	inline float distance(const Vector3 &v1, const Vector3 &v2)
	{
		return length(v1 - v2);
	}

	static const Vector3 UNIT_X(1.0f, 0.0f, 0.0f);
	static const Vector3 UNIT_Y(0.0f, 1.0f, 0.0f);
	static const Vector3 UNIT_Z(0.0f, 0.0f, 1.0f);
	static const Vector3 ZERO(0.0f, 0.0f, 0.0f);


	inline Vector3 findOrthogonal(const Vector3& v)
	{
		Vector3 u = UNIT_X;
		Vector3 n = normalize(v);
		if(dot(u, n) >= 0.99f)
			u = UNIT_Y;
		if(dot(u, n) >= 0.99f)
			u = UNIT_Z;
		return u - n * dot(u, n);
	}

}
