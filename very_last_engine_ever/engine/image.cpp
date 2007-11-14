#include "stdafx.h"
#include "image.h"

void engine::drawQuad(renderer::Device &device, Effect &fx, float x, float y, float w, float h, float s_nudge, float t_nudge)
{
	UINT passes;
	fx->Begin(&passes, 0);
	for (unsigned j = 0; j < passes; ++j)
	{
		fx->BeginPass(j);
		float verts[] =
		{
			x,     y,     0, 0 + s_nudge, 1 + t_nudge,
			x + w, y,     0, 1 + s_nudge, 1 + t_nudge,
			x + w, y + h, 0, 1 + s_nudge, 0 + t_nudge,
			x,     y + h, 0, 0 + s_nudge, 0 + t_nudge,
		};
		
		device->SetFVF(D3DFVF_XYZ | D3DFVF_TEX1);
		core::d3dErr(device->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, verts, sizeof(float) * 5));
		fx->EndPass();
	}
	fx->End();
}

void engine::Image::draw(renderer::Device &device)
{
	device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	eff->SetTexture("tex", tex);
	float s_nudge = 0.0f, t_nudge = 0.0f;
	float x_nudge = 0.0f, y_nudge = 0.0f;

	/* get render target */
	renderer::Surface rt = device.getRenderTarget();
	
	/* get viewport description */
	D3DVIEWPORT9 viewport = device.getViewport();

	/* setup nudge */
	x_nudge = -0.5f / (float(viewport.Width)  / 2);
	y_nudge =  0.5f / (float(viewport.Height) / 2);

	tex.getLevelDesc();

	/* get texture description */
	D3DSURFACE_DESC tex_desc;
	tex->GetLevelDesc(0, &tex_desc);

	/* setup nudge */
	s_nudge = 0.0f / tex_desc.Width;
	t_nudge = 0.0f / tex_desc.Height;

	/* draw quad */
	drawQuad(
		device, eff,
		x, y,
		w, h,
		s_nudge, t_nudge
	);
}
