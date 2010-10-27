#pragma once

#include "../core/comref.h"

namespace renderer
{

	class IndexBuffer : public ComRef<IDirect3DIndexBuffer9>
	{
	public:
		IndexBuffer(IDirect3DIndexBuffer9 *ib = NULL) : ComRef<IDirect3DIndexBuffer9>(ib) {}
		
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
