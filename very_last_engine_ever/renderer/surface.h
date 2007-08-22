#pragma once

namespace renderer
{
	class Surface : public CComPtr<IDirect3DSurface9>
	{
	public:
		Surface(IDirect3DSurface9 *surface = 0)
			: CComPtr<IDirect3DSurface9>(surface)
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
