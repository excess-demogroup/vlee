#pragma once

#include "transform.h"
#include "../math/vector3.h"

namespace scenegraph
{
	class TargetTransform : public Transform
	{
	public:
		TargetTransform(std::string name) : Transform(name), target(NULL), roll(0.0f) {}
		
		math::Matrix4x4 getLocalTransform()
		{
			if (NULL == target) return math::Matrix4x4::identity();
			
			math::Matrix4x4 eyeAbs = math::Matrix4x4::identity();
			if (NULL != getParent()) eyeAbs = getParent()->getAbsoluteTransform();
			math::Matrix4x4 targetAbs = target->getAbsoluteTransform();
			
			math::Vector3 targetPos = math::mul(targetAbs, math::Vector3(0, 0, 0));
			math::Vector3 eyePos = math::mul(eyeAbs, math::Vector3(0, 0, 0));
			
			return math::Matrix4x4::lookAt(eyePos, targetPos, roll);
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
		float roll;
	};
}
