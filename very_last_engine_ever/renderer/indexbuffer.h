#pragma once

namespace renderer
{

	class IndexBuffer : public CComPtr<IDirect3DIndexBuffer9>
	{
	public:
		IndexBuffer(IDirect3DIndexBuffer9 *ib = NULL) : CComPtr<IDirect3DIndexBuffer9>(ib) {}
		
		void *lock(UINT offset, UINT size, DWORD flags)
		{
			void *mem = 0;
			p->Lock(offset, size, &mem, flags);
			return mem;
		}

		void unlock()
		{
			p->Unlock();
		}
	};

}
