#pragma once

#include "transform.h"
#include "../math/vector3.h"
#include "../math/quaternion.h"

namespace scenegraph
{
	class PrsTransform : public Transform
	{
	public:
		PrsTransform(std::string name) :
			Transform(name),
			position(0,0,0),
			rotation(0,0,0),
			scale(0,0,0),
			matrix_dirty(true)
		{}
		
		math::Matrix4x4 getLocalTransform()
		{
			if (matrix_dirty)
			{
#if 0
				matrix = 
					math::Matrix4x4::translation(position) *
					math::Matrix4x4::rotation(rotation) *
					math::Matrix4x4::scaling(scale);
#else
				matrix = 
					math::Matrix4x4::scaling(scale) *
					math::Matrix4x4::rotation(rotation) *
					math::Matrix4x4::translation(position);
#endif
				matrix_dirty = false;
			}
			return matrix;
		}
		
		void setPosition(math::Vector3 &pos)
		{
			this->position = pos;
			this->matrix_dirty = true;
		}
		
		void setRotation(math::Quaternion &rot)
		{
			this->rotation = rot;
			this->matrix_dirty = true;
		}
		
		void setScale(math::Vector3 &scale)
		{
			this->scale = scale;
			this->matrix_dirty = true;
		}
		
	private:
		math::Vector3 position;
		math::Quaternion rotation;
		math::Vector3 scale;
		
		math::Matrix4x4 matrix;
		bool            matrix_dirty;
	};
}
