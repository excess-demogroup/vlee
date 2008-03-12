#pragma once

namespace math
{
	class Quaternion : public D3DXQUATERNION
	{
		Quaternion slerp(const Quaternion &q, float t) const
		{
			Quaternion res;
			D3DXQuaternionSlerp(&res, this, &q, t);
			return res;
		}
	};

}

