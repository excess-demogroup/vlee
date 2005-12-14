#pragma once

class Mesh : public CComPtr<ID3DXMesh>, public Drawable
{
	void draw()
	{
		p->DrawSubset(0);
	}
};
