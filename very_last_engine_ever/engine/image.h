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
		Image() : x(0), y(0), w(1), h(1)
		{
		}

		Image(renderer::Texture &tex, Effect &eff) :
			x(0), y(0),
			w(1), h(1),
			tex(tex), eff(eff)
		{
		}

		void draw(renderer::Device &device)
		{
			device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
			eff->SetTexture("tex", tex);
			float s_nudge = 0.0f, t_nudge = 0.0f;
			float x_nudge = 0.0f, y_nudge = 0.0f;

			/* get render target */
			renderer::Surface rt = device.getRenderTarget();
			
			/* get surface description */
			D3DSURFACE_DESC rt_desc;
			rt->GetDesc(&rt_desc);

			/* setup nudge */
			x_nudge = -0.5f / (float(rt_desc.Width)  / 2);
			y_nudge =  0.5f / (float(rt_desc.Height) / 2);

			/* get texture description */
			D3DSURFACE_DESC tex_desc;
			tex->GetLevelDesc(0, &tex_desc);

			/* setup nudge */
			s_nudge = 0.0f / tex_desc.Width;
			t_nudge = 0.0f / tex_desc.Height;

			UINT passes;
			eff->Begin(&passes, 0);
			for (unsigned j = 0; j < passes; ++j)
			{
				eff->BeginPass(j);
				float verts[] =
				{
					x+     x_nudge, y +     y_nudge, 0, 0 + s_nudge, 1 + t_nudge,
					x+ w + x_nudge, y +     y_nudge, 0, 1 + s_nudge, 1 + t_nudge,
					x+ w + x_nudge, y + h + y_nudge, 0, 1 + s_nudge, 0 + t_nudge,
					x+     x_nudge, y + h + y_nudge, 0, 0 + s_nudge, 0 + t_nudge,
				};
				
				device->SetFVF(D3DFVF_XYZ | D3DFVF_TEX1);
				core::d3dErr(device->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, verts, sizeof(float) * 5));
				eff->EndPass();
			}
			eff->End();
		}
		float x, y;
		float w, h;

		renderer::Texture tex;
		Effect  eff;
	};


}
