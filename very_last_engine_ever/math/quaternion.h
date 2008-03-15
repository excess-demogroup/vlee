#pragma once

namespace math
{
	class Quaternion : public D3DXQUATERNION
	{
	public:
		Quaternion() {}
		
		Quaternion(float x, float y, float z)
		{
			Vector3 xaxis(1, 0, 0), yaxis(0, 1, 0), zaxis(0, 0, 1);
			Quaternion xrot, yrot, zrot;
			D3DXQuaternionRotationAxis(&xrot, &xaxis, x);
			D3DXQuaternionRotationAxis(&yrot, &yaxis, y);
			D3DXQuaternionRotationAxis(&zrot, &zaxis, z);
			*this = identity();
			*this *= xrot;
			*this *= yrot;
			*this *= zrot;
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

