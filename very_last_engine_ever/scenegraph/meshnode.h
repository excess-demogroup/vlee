#pragma once

#include "drawable.h"
#include "../engine/mesh.h"
#include "../engine/effect.h"

namespace scenegraph
{
	class MeshNode : public Drawable
	{
	public:
		MeshNode(std::string name, engine::Mesh *mesh, engine::Effect *effect) :
			Drawable(name),
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
			effect->draw(*mesh);
		}
		
		bool transparent;
		engine::Effect *effect;
		engine::Mesh   *mesh;
	};
}
