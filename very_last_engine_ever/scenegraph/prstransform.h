#pragma once

#include "transform.h"

namespace scenegraph
{
	class PrsTransform : public Transform
	{
	public:
		PrsTransform() :
			position(0,0,0),
			rotation(0,0,0),
			scale(0,0,0),
			matrix_dirty(true)
		{}

		math::Matrix4x4 getTransform()
		{
			if (matrix_dirty)
			{
				math::Matrix4x4 mposition;
				math::Matrix4x4 mrotation;
				math::Matrix4x4 mscale;

				mposition.make_translation(position);
				mrotation.make_rotation(rotation);
				mscale.make_scaling(scale);

				matrix = mposition * mrotation * mscale;
				matrix_dirty = false;
			}
			return matrix;
		}

	private:
		math::Vector3 position;
		math::Vector3 rotation;
		math::Vector3 scale;
		
		math::Matrix4x4 matrix;
		bool      matrix_dirty;
	};
}
