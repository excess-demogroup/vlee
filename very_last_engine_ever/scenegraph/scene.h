#pragma once

#include "node.h"
#include "camera.h"
#include "prstransform.h"

namespace scenegraph
{

	class PrsAnim
	{
	public:
		math::Vector3    getPos(float time);
		math::Quaternion getRot(float time);
		math::Vector3    getScale(float time);
		
		void setPosKeyFrame(float time, const math::Vector3 &pos)
		{
			posTrack[time] = pos;
		}
		
		void setRotKeyFrame(float time, const math::Quaternion &rot)
		{
			rotTrack[time] = rot;
		}
		
		void setScaleKeyFrame(float time, const math::Vector3 &scale)
		{
			scaleTrack[time] = scale;
		}
		
	private:
		std::map<float, math::Vector3> posTrack;
		std::map<float, math::Quaternion> rotTrack;
		std::map<float, math::Vector3> scaleTrack;
	};

	class Scene : public Node
	{
	public:
		Scene(std::string name) : Node(name) {}
		
		virtual NodeType getType() { return NODE_SCENE; }
		
		void anim(float time);
		
		Camera *findCamera(const std::string &name)
		{
			return findNodeByType<Camera, NODE_CAMERA>(name);
		}

		Node *findNode(const std::string &name)
		{
			return findChild(name);
		}
		
		void addPrsAnim(PrsTransform *node, const PrsAnim &anim)
		{
			animTracks[node] = anim;
		}
		
	private:
		std::map<PrsTransform*, PrsAnim> animTracks;

		template <typename T, NodeType t>
		T *findNodeByType(const std::string &name)
		{
			// find node
			Node *node = findChild(name);
			if (NULL == node) return NULL;
			
			// type check
			if (node->getType() != t) return NULL;
			return reinterpret_cast<T*>(node);
		}
	};
}