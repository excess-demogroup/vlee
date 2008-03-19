#pragma once

#include "core/err.h"
#include "renderer/device.h"
#include "renderer/texture.h"

#include "engine/effect.h"
#include "engine/drawable.h"
#include "engine/mesh.h"

namespace engine
{
	void drawQuad(renderer::Device &device, Effect *fx, float x, float y, float w, float h, float s_nudge = 0.0f, float t_nudge = 0.0f);
	
	class Image
	{
	public:
		Image() : x(0), y(0), w(1), h(1), eff(NULL)
		{
		}

		Image(renderer::Texture &tex, Effect *eff) :
			x(0), y(0),
			w(1), h(1),
			tex(tex), eff(eff)
		{
		}

		void draw(renderer::Device &device);

		void setPosition(float x, float y)
		{
			this->x = x;
			this->y = y;
		}

		void setDimension(float w, float h)
		{
			this->w = w;
			this->h = h;
		}
		
		void setTexture(renderer::Texture &tex)
		{
			this->tex = tex;
		}
		
	private:
		float x, y;
		float w, h;

		renderer::Texture tex;
		Effect *eff;
	};


}
