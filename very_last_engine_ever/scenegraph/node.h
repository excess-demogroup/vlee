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
		Node(const std::string &name) : name(name) { assert(name.length() > 0); }
		virtual ~Node();
		
		void addChild(Node *node)
		{
			children.push_back(node);
		}
		
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
		
		typedef std::list<Node*>::iterator       child_iterator;
		typedef std::list<Node*>::const_iterator child_const_iterator;
		child_iterator beginChildren() { return children.begin(); }
		child_iterator endChildren()   { return children.end();   }
		child_const_iterator beginChildren() const { return children.begin(); }
		child_const_iterator endChildren()   const { return children.end();   }
		
	private:
		const std::string name;
		
		std::list<Node*> children;
	};
}
