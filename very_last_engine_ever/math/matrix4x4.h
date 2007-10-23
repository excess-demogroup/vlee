#pragma once

#include "vector3.h"

namespace math
{
	class Matrix4x4 : public D3DXMATRIX
	{
	public:
		Matrix4x4() {}
		Matrix4x4(const Matrix4x4 &mat) : D3DXMATRIX(mat) { }
		Matrix4x4(const D3DXMATRIX &mat) : D3DXMATRIX(mat) { }

		void make_identity()
		{
			D3DXMatrixIdentity(this);
		}

		void make_translation(const Vector3 &translate)
		{
			D3DXMatrixTranslation(this, translate.x, translate.y, translate.z);
		}
		void make_rotation(const Vector3 &rotation)
		{
			D3DXMatrixRotationYawPitchRoll(this, rotation.x, rotation.y, rotation.z);
		}

		void make_scaling(const Vector3 &scale)
		{
			D3DXMatrixScaling(this, scale.x, scale.y, scale.z);
		}
		
		void make_projection(float fov, float aspect, float znear, float zfar)
		{
			D3DXMatrixPerspectiveFovLH(this, D3DXToRadian(fov), aspect, znear, zfar);
		}
	};

	inline Vector3 mul(const Matrix4x4 &m, const Vector3 &v)
	{
		D3DXVECTOR4 temp;
		D3DXVec3Transform(
			&temp,
			&v,
			&m
		);
		return Vector3(temp.x, temp.y, temp.z);
	}

}
