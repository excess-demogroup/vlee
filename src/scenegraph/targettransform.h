#pragma once

#include "transform.h"
#include "../math/vector3.h"

namespace scenegraph
{
	class TargetTransform : public Transform
	{
	public:
		TargetTransform(std::string name) : Transform(name), target(NULL) {}
		
		math::Matrix4x4 getLocalTransform()
		{
			if (NULL == target) return math::Matrix4x4::identity();
			
			math::Matrix4x4 eyeAbs = math::Matrix4x4::identity();
			if (NULL != getParent())
			{
				eyeAbs = getParent()->getAbsoluteTransform();
			}
			
			// get target position into the local coordinate system
			// note: this will obviously fail horribly if target is dependent on this node
			math::Matrix4x4 eyeAbsInv = eyeAbs.inverse();

			printf("target: %f %f %f\n",
				target->getAbsoluteTransform().getTranslation().x,
				target->getAbsoluteTransform().getTranslation().y,
				target->getAbsoluteTransform().getTranslation().z
			);

			math::Vector3 targetPos = math::mul(eyeAbsInv, target->getAbsoluteTransform().getTranslation());
			
			math::Matrix4x4 lookAt = math::Matrix4x4::lookAt(math::Vector3(0, 0, 0), targetPos, 0.0f);
			return lookAt.inverse();
		}
		
		void setTarget(Node *target)
		{
			this->target = target;
		}
		
		Node *getTarget() const
		{
			return target;
		}
		
	private:
		Node *target;
	};
}
