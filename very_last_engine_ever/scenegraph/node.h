#pragma once

namespace scenegraph
{
	enum NodeType
	{
		NODE_SCENE,
		NODE_CAMERA,
		NODE_LIGHT,
		NODE_TRANSFORM,
		NODE_DRAWABLE,
	};

	class Node
	{
	public:

		Node() : parent(NULL) { }
		
		virtual ~Node();

		void addChildren(Node *node)
		{
			children.push_back(node);
		}
		
		Node *getParent() const
		{
			return parent; // who's your daddy?!
		}

		virtual NodeType getType() = 0;

	private:

		Node *parent;
		std::list<Node*> children;
	};
}
