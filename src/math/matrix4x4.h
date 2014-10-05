#pragma once

#include "vector3.h"
#include "quaternion.h"

namespace math
{
	class Matrix4x4 : public D3DXMATRIX
	{
	public:
		Matrix4x4() {}
		Matrix4x4(const Matrix4x4 &mat) : D3DXMATRIX(mat) { }
		Matrix4x4(const D3DXMATRIX &mat) : D3DXMATRIX(mat) { }
		
		static Matrix4x4 identity()
		{
			Matrix4x4 ret;
			ret.makeIdentity();
			return ret;
		}
		
		static Matrix4x4 translation(const Vector3 &translate)
		{
			Matrix4x4 ret;
			ret.makeTranslation(translate);
			return ret;
		}
		
		static Matrix4x4 rotation(const Vector3 &rotate)
		{
			Matrix4x4 ret;
			ret.makeRotation(rotate);
			return ret;
		}
		
		static Matrix4x4 rotation(const Quaternion &rotate)
		{
			Matrix4x4 ret;
			D3DXMatrixRotationQuaternion(&ret, &rotate);
			return ret;
		}
		
		static Matrix4x4 scaling(const Vector3 &scale)
		{
			Matrix4x4 ret;
			ret.makeScaling(scale);
			return ret;
		}
		
		static Matrix4x4 projection(float fov, float aspect, float znear, float zfar)
		{
			Matrix4x4 ret;
			ret.makeProjection(fov, aspect, znear, zfar);
			return ret;
		}

		static Matrix4x4 lookAt(const Vector3 &eye, const Vector3 &target, float roll)
		{
			Matrix4x4 ret;
			ret.makeLookAt(eye, target, roll);
			return ret;
		}
		
		void makeIdentity()
		{
			D3DXMatrixIdentity(this);
		}
		
		void makeTranslation(const Vector3 &translate)
		{
			D3DXMatrixTranslation(this, translate.x, translate.y, translate.z);
		}
		
		void makeRotation(const Vector3 &rotation)
		{
			D3DXMatrixRotationYawPitchRoll(this, rotation.y, rotation.x, rotation.z);
		}
		
		void makeScaling(const Vector3 &scale)
		{
			D3DXMatrixScaling(this, scale.x, scale.y, scale.z);
		}
		
		void makeProjection(float fov, float aspect, float znear, float zfar)
		{
			D3DXMatrixPerspectiveFovLH(this, D3DXToRadian(fov), aspect, znear, zfar);
		}
		
		void makeLookAt(const Vector3 &eye, const Vector3 &target, float roll)
		{
			D3DXVECTOR3 up(0, 1, 0);
			D3DXMatrixLookAtLH(this, &eye, &target, &up);
			*this *= rotation(Vector3(0, 0, roll));;
		}
		
		Matrix4x4 inverse() const
		{
			Matrix4x4 res;
			D3DXMatrixInverse(&res, NULL, this);
			return res;
		}

		Matrix4x4 transpose() const
		{
			Matrix4x4 res;
			D3DXMatrixTranspose(&res, this);
			return res;
		}

		Vector3 getTranslation() const
		{
			return Vector3(_41, _42, _43);
		}
		
		Vector3 getZAxis() const
		{
			return Vector3(_13, _23, _33);
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
