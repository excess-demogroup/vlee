#pragma once

#include "surface.h"

namespace renderer
{
	class Device : public CComPtr<IDirect3DDevice9>
	{
	public:

		void set_render_target(Surface &surface, unsigned index = 0)
		{
			assert(NULL != p);
			assert(NULL != surface);
			p->SetRenderTarget(index, surface);
		}

		Surface get_render_target(unsigned index = 0)
		{
			assert(NULL != p);
			IDirect3DSurface9 *surface;
			p->GetRenderTarget(index, &surface);
			Surface s;
//			s->Attach(surface);
			return Surface(surface);
		}
	};
};
