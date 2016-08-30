#pragma once

namespace math
{
	class Vector2 : public D3DXVECTOR2
	{
	public:
		Vector2(double x, double y) : D3DXVECTOR2(float(x), float(y)) {}
		Vector2(const D3DXVECTOR2 &v) : D3DXVECTOR2(v) {}
	};
}
