#pragma once

namespace scenegraph
{
	class Node {
	public:

		Node() : parent(0) { }

		virtual ~Node();

		Node *getParent() const {
			return parent; // who's your daddy?!
		}

	private:

		Node *parent;
		std::list<Node*> children;

	};
}
