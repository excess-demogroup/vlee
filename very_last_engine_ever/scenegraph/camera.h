#pragma once

#include "node.h"
#include "math/matrix4x4.h"

namespace scenegraph
{
	class Camera : public Node
	{
	public:
		Camera(std::string name) :
			Node(name),
			fov(60.0f), aspect(4.0f / 3),
			znear(1.0f), zfar(1000.0f),
			projection_dirty(true)
		{
		}
		
		virtual NodeType getType() { return NODE_CAMERA; }
		
		void setFov(float fov)
		{
			this->fov = fov;
			projection_dirty = true;
		}
		
		void setAspect(float aspect)
		{
			this->aspect = aspect;
			projection_dirty = true;
		}
		
		const math::Matrix4x4 &getProjection() const
		{
			/* recalculate matrix if needed */
			if (projection_dirty)
			{
				projection.make_projection(fov, aspect, znear, zfar);
				projection_dirty = false;
			}
			
			return projection;
		}
		
	private:
		float fov;
		float aspect;
		float znear, zfar;
		
		mutable math::Matrix4x4 projection;
		mutable bool            projection_dirty;
	};
}