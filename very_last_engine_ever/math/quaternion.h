#pragma once

namespace math
{
	class Quaternion : public D3DXQUATERNION
	{
	public:
		Quaternion() {}
		
		Quaternion(float x, float y, float z)
		{
			D3DXQuaternionRotationYawPitchRoll(this, x, y, z);
		}
		
		static Quaternion identity()
		{
			Quaternion ret;
			D3DXQuaternionIdentity(&ret);
			return ret;
		}
		
		Quaternion slerp(const Quaternion &q, float t) const
		{
			Quaternion res;
			D3DXQuaternionSlerp(&res, this, &q, t);
			return res;
		}
	};
}

