#pragma once

#include "transform.h"

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
		
		math::Matrix4x4 getTransform()
		{
			if (matrix_dirty)
			{
				matrix = 
					math::Matrix4x4::translation(position) *
					math::Matrix4x4::rotation(rotation) *
					math::Matrix4x4::scaling(scale);
				matrix_dirty = false;
			}
			return matrix;
		}
		
		void setPosition(math::Vector3 &pos)
		{
			this->position = pos;
			this->matrix_dirty = true;
		}
		
		void setRotation(math::Vector3 &rot)
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
		math::Vector3 rotation;
		math::Vector3 scale;
		
		math::Matrix4x4 matrix;
		bool            matrix_dirty;
	};
}
