#pragma once

#include "drawable.h"
#include "../renderer/device.h"
#include "../renderer/texture.h"
#include "../renderer/cubetexture.h"
#include "../renderer/volumetexture.h"
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
			assert( NULL != p );
			
			world      = p->GetParameterBySemantic(0, "WORLD");
			view       = p->GetParameterBySemantic(0, "VIEW");
			projection = p->GetParameterBySemantic(0, "PROJECTION");
			worldview  = p->GetParameterBySemantic(0, "WORLDVIEW");
			worldviewprojection = p->GetParameterBySemantic(0, "WORLDVIEWPROJECTION");
		}

		void commitChanges()
		{
			assert( NULL != p );
			p->CommitChanges();
		}
		
		void setMatrix(D3DXHANDLE param, const math::Matrix4x4 &mat)
		{
			assert( NULL != p );
			p->SetMatrix(param, &mat);
		}
		
		void setFloat(D3DXHANDLE param, const float f)
		{
			assert( NULL != p );
			p->SetFloat(param, f);
		}

		void setFloatArray(D3DXHANDLE param, const float *f, size_t count)
		{
			assert( NULL != p );
			p->SetFloatArray(param, f, UINT(count));
		}
		
		void setVector3(D3DXHANDLE param, const math::Vector3 &v)
		{
			assert( NULL != p );
			D3DXVECTOR4 v4(v.x, v.y, v.z, 1.0);
			p->SetVector(param, &v4);
		}
		
		void setTexture(D3DXHANDLE param, renderer::Texture &texture)
		{
			p->SetTexture(param, texture);
		}
		
		void setTexture(D3DXHANDLE param, renderer::CubeTexture &texture)
		{
			p->SetTexture(param, texture);
		}
		
		void setTexture(D3DXHANDLE param, renderer::VolumeTexture &texture)
		{
			p->SetTexture(param, texture);
		}
		
		void setMatrices(D3DXMATRIX world, D3DXMATRIX view, D3DXMATRIX proj)
		{
			assert( NULL != p );
			D3DXMATRIX world_view_proj;
			world_view_proj = world * view * proj;
			
			D3DXMATRIX world_view;
			world_view = world * view;
			
			if (this->world != NULL) setMatrix(this->world,      world);
			setMatrix(this->view,       view);
			setMatrix(this->projection, proj);
			setMatrix(this->worldview,  world_view);
			setMatrix(this->worldviewprojection, world_view_proj);
		}

		void draw(Drawable &d)
		{
//			assert( NULL != p );
			UINT passes = 0;
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

	Effect *loadEffect(renderer::Device &device, std::string filename);
}
