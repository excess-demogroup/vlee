#pragma once

namespace scenegraph
{
	class Drawable
	{
	public:
		virtual NodeType getType() { return NODE_DRAWABLE; }
		
		virtual bool isTransparent() = 0;
		virtual void draw() = 0;
	};
}
