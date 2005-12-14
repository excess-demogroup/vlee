#pragma once

class Effect : public CComPtr<ID3DXEffect>
{
public:
	Effect() : CComPtr<ID3DXEffect>(), world(0), view(0), projection(0), worldview(0), worldviewprojection(0) { }

	void update()
	{
		assert(p != 0);
		world      = p->GetParameterBySemantic(0, "WORLD");
		view       = p->GetParameterBySemantic(0, "VIEW");
		projection = p->GetParameterBySemantic(0, "PROJECTION");
		worldview  = p->GetParameterBySemantic(0, "WORLDVIEW");
		worldviewprojection = p->GetParameterBySemantic(0, "WORLDVIEWPROJECTION");
	}

	void set_matrices(D3DXMATRIX world, D3DXMATRIX view, D3DXMATRIX proj)
	{
			D3DXMATRIX world_view_proj;
			world_view_proj = world * view * proj;

			D3DXMATRIX world_view;
			world_view = world * view;

			assert(p != 0);

			p->SetMatrix(this->world,      &world);
			p->SetMatrix(this->view,       &view);
			p->SetMatrix(this->projection, &proj);
			p->SetMatrix(this->worldview,  &world_view);
			p->SetMatrix(this->worldviewprojection, &world_view_proj);
	}

	void draw(Drawable &d)
	{
		UINT passes;
		p->Begin(&passes, 0);
		for (unsigned j = 0; j < passes; ++j)
		{
			p->BeginPass(j);
			d.draw();
			p->EndPass();
		}
		p->End();
	}

	D3DXHANDLE world, view, projection, worldview, worldviewprojection;
};
