#pragma once

#include "../core/fatalexception.h"

namespace renderer
{
	class VertexBuffer : public CComPtr<IDirect3DVertexBuffer9>
	{
	public:
		VertexBuffer() : CComPtr<IDirect3DVertexBuffer9>() {}

		VertexBuffer(Device &device, UINT length, DWORD usage, DWORD fvf, D3DPOOL pool = D3DPOOL_DEFAULT, HANDLE* handle = 0)
		{
			assert(NULL != device);
			core::log::printf("creating vertexbuffer... ");

			IDirect3DVertexBuffer9 *vb;
			if (FAILED(device->CreateVertexBuffer(length, usage, fvf, pool, &vb, handle)))
			{
				throw core::FatalException("failed to create vertex buffer");
			}
			assert(NULL != vb);
			core::log::printf("done.\n");
			Attach(vb); // don't addref
		}

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
