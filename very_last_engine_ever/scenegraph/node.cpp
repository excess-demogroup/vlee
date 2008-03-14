#include "stdafx.h"

#include "node.h"
using namespace scenegraph;

Node::~Node()
{
	// make sure parent don't reference to junk
/*	if (parent) parent->children.remove(this);
	parent = 0; */

	// make sure children don't reference to junk as well
/*	std::list<Node*>::iterator i;
	for (i = children.begin(); i != children.end(); ++i)
	{
		(*i)->parent = 0;
	} */
}
