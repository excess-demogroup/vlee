#pragma once

#include "drawable.h"
#include "../engine/mesh.h"
#include "../engine/effect.h"

namespace scenegraph
{
	class MeshNode : public DrawableNode
	{
	public:
		MeshNode(std::string name, engine::Mesh *mesh, engine::Effect *effect) :
			DrawableNode(name),
			mesh(mesh),
			effect(effect),
			transparent(false)
		{}

		bool isTransparent()
		{
			return transparent;
		}
		
		void draw()
		{
			assert(NULL != effect);
			assert(NULL != mesh);
			effect->draw(*mesh);
		}
		
		bool transparent;
		engine::Effect *effect;
		engine::Mesh   *mesh;
	};
}
