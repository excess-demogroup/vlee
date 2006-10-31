#pragma once

#include "core/err.h"
#include "renderer/device.h"

#include "engine/effect.h"
#include "engine/drawable.h"
#include "engine/mesh.h"

namespace engine
{

	class Image
	{
	public:
		Image()
		{
		}

		Image(renderer::Texture &tex, Effect &eff, Mesh &poly) :
			tex(tex), eff(eff), poly(poly)
		{
			/* nuthin' */
		}

		void draw(Device &device)
		{
			device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
			eff->SetTexture("tex", tex);
			eff->CommitChanges();
			eff.draw(poly);
		}

		renderer::Texture tex;
		Effect  eff;
		Mesh    poly;
	};


}
