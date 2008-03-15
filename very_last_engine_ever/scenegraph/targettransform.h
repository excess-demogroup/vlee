#pragma once

#include "transform.h"
#include "../math/vector3.h"

namespace scenegraph
{
	class TargetTransform : public Transform
	{
	public:
		TargetTransform(std::string name) : Transform(name), target(NULL) {}
		
		math::Matrix4x4 getTransform()
		{
			return math::Matrix4x4::identity();
		}
		
		void setTarget(Node *target)
		{
			this->target = target;
		}
		
		Node *getTarget()
		{
			return target;
		}
		
	private:
		Node *target;
	};
}
