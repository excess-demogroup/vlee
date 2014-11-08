#pragma once

#include "../core/comref.h"
#include "drawable.h"
#include "../renderer/device.h"
#include "../renderer/texture.h"
#include "../renderer/cubetexture.h"
#include "../renderer/volumetexture.h"
#include "../math/matrix4x4.h"
#include "../math/vector3.h"
#include "../math/vector2.h"

namespace engine {
	class Effect : public ComRef<ID3DXEffect> {
	public:
		Effect() :
			ComRef<ID3DXEffect>(), world(NULL), view(NULL),
			projection(NULL), worldview(NULL),
			worldview_inv(NULL), worldviewprojection(NULL)
		{
			// nothing
		}
		
		void update()
		{
			assert( NULL != p );
			
			world      = p->GetParameterBySemantic(0, "WORLD");
			world_inv  = p->GetParameterBySemantic(0, "WORLDINVERSE");
			view       = p->GetParameterBySemantic(0, "VIEW");
			view_inv   = p->GetParameterBySemantic(0, "VIEWINVERSE");
			projection = p->GetParameterBySemantic(0, "PROJECTION");
			worldview  = p->GetParameterBySemantic(0, "WORLDVIEW");
			worldview_inv = p->GetParameterBySemantic(0, "WORLDVIEWINVERSE");
			worldviewprojection = p->GetParameterBySemantic(0, "WORLDVIEWPROJECTION");
			viewPos   = p->GetParameterBySemantic(0, "VIEWPOSITION");
			viewDir   = p->GetParameterBySemantic(0, "VIEWDIRECTION");
			matWVP_inv = p->GetParameterBySemantic(0, "WORLDVIEWPROJECTIONINV");
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

		void setVector2(D3DXHANDLE param, const math::Vector2 &v)
		{
			assert( NULL != p );
			D3DXVECTOR4 v4(v.x, v.y, 0.0, 1.0);
			p->SetVector(param, &v4);
		}

		void setVector3(D3DXHANDLE param, const math::Vector3 &v)
		{
			assert( NULL != p );
			D3DXVECTOR4 v4(v.x, v.y, v.z, 1.0);
			p->SetVector(param, &v4);
		}

		void setTexture(D3DXHANDLE param, const renderer::Texture &texture)
		{
			p->SetTexture(param, texture.tex);
		}
		
		void setTexture(D3DXHANDLE param, const renderer::CubeTexture &texture)
		{
			p->SetTexture(param, texture.tex);
		}
		
		void setTexture(D3DXHANDLE param, const renderer::VolumeTexture &texture)
		{
			p->SetTexture(param, texture);
		}
		
		void setMatrices(const math::Matrix4x4 &world, const math::Matrix4x4 &view, const math::Matrix4x4 &proj)
		{
			assert( NULL != p );
			math::Matrix4x4 world_view_proj, matWVP_inv;
			world_view_proj = world * view * proj;
			matWVP_inv = world_view_proj.inverse();

			math::Matrix4x4 world_view;
			world_view = world * view;
			math::Matrix4x4 world_inv = world.inverse(), world_view_inv = world_view.inverse();

			if (this->world != NULL)
				setMatrix(this->world, world);
			if (this->world_inv != NULL)
				setMatrix(this->world_inv, world_inv);
			if (this->view  != NULL)
				setMatrix(this->view, view);
			if (this->view_inv != NULL)
				setMatrix(this->view, view.inverse());
			if (this->projection != NULL)
				setMatrix(this->projection, proj);
			if (this->worldview != NULL)
				setMatrix(this->worldview, world_view);
			if (this->worldview_inv != NULL)
				setMatrix(this->worldview_inv, world_view_inv);
			if (this->worldviewprojection != NULL)
				setMatrix(this->worldviewprojection, world_view_proj);
			if (this->matWVP_inv)
				setMatrix(this->matWVP_inv, matWVP_inv);

			if (this->viewPos != NULL) setVector3(this->viewPos,  world_view_inv.getTranslation());
			if (this->viewDir != NULL) setVector3(this->viewDir,  world_view.getZAxis());
		}

		void draw(Drawable *d)
		{
//			assert( NULL != p );
			UINT passes = 0;
			p->Begin(&passes, 0);
			for (unsigned j = 0; j < passes; ++j)
			{
				p->BeginPass(j);
				d->draw();
				p->EndPass();
			}
			p->End();
		}

		D3DXHANDLE world, world_inv, view, view_inv, projection, worldview, worldview_inv, worldviewprojection, matWVP_inv;
		D3DXHANDLE viewPos, viewDir;
	};

	Effect *loadEffect(renderer::Device &device, std::string filename);
}
