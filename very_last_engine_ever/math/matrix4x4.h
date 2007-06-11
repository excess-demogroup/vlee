#pragma once

#include "vector3.h"

namespace math
{
	class Matrix4x4 : public D3DXMATRIX
	{
	public:
		Matrix4x4() {}

		Matrix4x4(const Matrix4x4 &mat)
		{
			memcpy(this, &mat, sizeof(Matrix4x4));
		}

		Matrix4x4(const D3DXMATRIX &mat)
		{
			memcpy(this, &mat, sizeof(Matrix4x4));
		}

		void make_identity()
		{
			D3DXMatrixIdentity(this);
		}

		void make_scaling(const Vector3 &scale)
		{
			D3DXMatrixScaling(this, scale.x, scale.y, scale.z);
		}

		void make_translation(const Vector3 &translate)
		{
			D3DXMatrixTranslation(this, translate.x, translate.y, translate.z);
		}
		
		void make_projection(float fov, float aspect, float znear, float zfar)
		{
			D3DXMatrixPerspectiveFovLH(this, D3DXToRadian(fov), aspect, znear, zfar);
		}
	};
}
