#pragma once

#include "node.h"
#include "math/vector3.h"

namespace scenegraph
{
	class Target : public Node
	{
	public:
		Target(std::string name) : Node(name) {}
		
		virtual NodeType getType() { return NODE_TARGET; }
		
		const math::Vector3 &getTarget() const { return target; }
		void setTarget(const math::Vector3 &target) { this->target = target; }
	private:
		math::Vector3 target;
	};
}
