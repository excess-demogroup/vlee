#pragma once

class Node {
public:

	Node() : parent(0) { }

	virtual ~Node();

	Node *get_parent() const {
		return parent; // who's your daddy?!
	}

private:

	Node *parent;
	std::list<Node*> children;

};
