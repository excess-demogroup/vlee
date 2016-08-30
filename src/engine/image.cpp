#include "stdafx.h"
#include "image.h"

void engine::drawQuad(renderer::Device &device, Effect *fx, float x, float y, float w, float h)
{
	D3DVIEWPORT9 viewport = device.getViewport();
	x -= 1.0f / viewport.Width;
	y += 1.0f / viewport.Height;

	const float verts[] = {
		x,     y,     0.5f, 0.0f, 1.0f,
		x + w, y,     0.5f, 1.0f, 1.0f,
		x + w, y + h, 0.5f, 1.0f, 0.0f,
		x,     y + h, 0.5f, 0.0f, 0.0f,
	};

	UINT passes;
	(*fx)->Begin(&passes, 0);
	for (unsigned j = 0; j < passes; ++j) {
		(*fx)->BeginPass(j);
		device->SetFVF(D3DFVF_XYZ | D3DFVF_TEX1);
		core::d3dErr(device->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, verts, sizeof(float) * 5));
		(*fx)->EndPass();
	}
	(*fx)->End();
}
