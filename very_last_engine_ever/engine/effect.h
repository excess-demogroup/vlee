#pragma once

#include "drawable.h"
#include "../renderer/device.h"
#include "../math/matrix4x4.h"
#include "../math/vector3.h"

namespace engine
{

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
		
		void setMatrix(D3DXHANDLE param, const math::Matrix4x4 &mat)
		{
			p->SetMatrix(param, &mat);
		}
		
		void setVector3(D3DXHANDLE param, const math::Vector3 &v)
		{
			D3DXVECTOR4 v4(v.x, v.y, v.z, 1.0);
			p->SetVector(param, &v4);
		}
		
		void setMatrices(D3DXMATRIX world, D3DXMATRIX view, D3DXMATRIX proj)
		{
			D3DXMATRIX world_view_proj;
			world_view_proj = world * view * proj;
			
			D3DXMATRIX world_view;
			world_view = world * view;
			
			assert(p != 0);
			
			setMatrix(this->world,      world);
			setMatrix(this->view,       view);
			setMatrix(this->projection, proj);
			setMatrix(this->worldview,  world_view);
			setMatrix(this->worldviewprojection, world_view_proj);
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

	Effect loadEffect(renderer::Device &device, std::string filename);
}
