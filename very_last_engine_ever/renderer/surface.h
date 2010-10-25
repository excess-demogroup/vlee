#pragma once

#include "core/comref.h"

namespace renderer
{
	class Surface : public ComRef<IDirect3DSurface9>
	{
	public:
		Surface(IDirect3DSurface9 *surface = 0)
			: ComRef<IDirect3DSurface9>(surface)
		{
		}

		const D3DSURFACE_DESC getDesc() const
		{
			D3DSURFACE_DESC desc;
			p->GetDesc(&desc);
			return desc;
		}
	};

}
