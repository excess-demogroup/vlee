#pragma once

namespace math
{
	class Vector2 : public D3DXVECTOR2
	{
	public:
		Vector2(double x, double y) : D3DXVECTOR2(float(x), float(y))
		{

		}
	};
}
