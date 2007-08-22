#pragma once

#include "../core/fatalexception.h"
#include "../core/log.h"
#include "device.h"

namespace renderer
{
	class VertexBuffer : public CComPtr<IDirect3DVertexBuffer9>
	{
	public:
		VertexBuffer(IDirect3DVertexBuffer9 *vertex_buffer = NULL) : CComPtr<IDirect3DVertexBuffer9>(vertex_buffer) {}
		
		void *lock(UINT offset, UINT size, DWORD flags)
		{
			assert(NULL != p);
			void *mem = 0;
			p->Lock(offset, size, &mem, flags);
			assert(NULL != mem);
			return mem;
		}

		void unlock()
		{
			p->Unlock();
		}
	};
}
