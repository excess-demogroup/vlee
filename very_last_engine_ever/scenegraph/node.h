#pragma once

#include <string>

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
		Node(std::string name) /*: parent(NULL)  */{ }
		virtual ~Node();
		
		void addChild(Node *node)
		{
			children.push_back(node);
		}
		
/*		Node *getParent() const
		{
			return parent; // who's your daddy?!
		} */
		
		virtual NodeType getType() = 0;
		
		typedef std::list<Node*>::iterator       child_iterator;
		typedef std::list<Node*>::const_iterator child_const_iterator;
		child_iterator beginChildren() { return children.begin(); }
		child_iterator endChildren()   { return children.end();   }
		child_const_iterator beginChildren() const { return children.begin(); }
		child_const_iterator endChildren()   const { return children.end();   }
		
	private:
		std::string name;
		
//		Node *parent;
		std::list<Node*> children;
	};
}
