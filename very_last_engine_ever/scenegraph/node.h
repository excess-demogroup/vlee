#pragma once

#include <string>
#include "../math/matrix4x4.h"

namespace scenegraph
{
	enum NodeType
	{
		NODE_SCENE,
		NODE_CAMERA,
		NODE_LIGHT,
		NODE_TRANSFORM,
		NODE_DRAWABLE,
		NODE_TARGET,
	};
	
	class Node
	{
	public:
		Node(const std::string &name) : parent(NULL), name(name) { assert(name.length() > 0); }
		virtual ~Node();
		
		void addChild(Node *node)
		{
			children.push_back(node);
			node->setParent(this);
		}
		
		void setParent(Node *node) { parent = node; }
		Node *getParent() { return parent; }
		
		Node *findChild(const std::string &name)
		{
			child_iterator i;
			for (i = beginChildren(); i != endChildren(); ++i)
			{
				printf("looking for \"%s\", checking \"%s\"\n", name.c_str(), (*i)->name.c_str());
				if ((*i)->name == name) return *i;
			}
			
			// not found, iterate through all children
			for (i = beginChildren(); i != endChildren(); ++i)
			{
				Node *n = (*i)->findChild(name);
				if (NULL != n) return n;
			}
			
			// nothing found
			return NULL;
		}
		
		virtual NodeType getType() = 0;
		virtual math::Matrix4x4 getLocalTransform() { return math::Matrix4x4::identity(); };

		math::Matrix4x4 getAbsoluteTransform()
		{
			math::Matrix4x4 absoluteTransform = getLocalTransform();
			
			Node *curr = this;
			while (NULL != curr->getParent())
			{
				curr = curr->parent;
				absoluteTransform *= curr->getLocalTransform();
			}
			
			return absoluteTransform;
		}
		
		typedef std::list<Node*>::iterator       child_iterator;
		typedef std::list<Node*>::const_iterator child_const_iterator;
		child_iterator beginChildren() { return children.begin(); }
		child_iterator endChildren()   { return children.end();   }
		child_const_iterator beginChildren() const { return children.begin(); }
		child_const_iterator endChildren()   const { return children.end();   }
		
	private:
		const std::string name;
		
		Node *parent;
		std::list<Node*> children;
	};
}
