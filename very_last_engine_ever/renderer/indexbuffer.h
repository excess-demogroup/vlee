#pragma once

namespace renderer
{

	class IndexBuffer : public CComPtr<IDirect3DIndexBuffer9>
	{
	public:
		IndexBuffer() : CComPtr<IDirect3DIndexBuffer9>() {}

		IndexBuffer(Device &device, UINT length, DWORD usage, D3DFORMAT format, D3DPOOL pool = D3DPOOL_DEFAULT, HANDLE* handle = 0)
		{
			assert(NULL != device);
			core::log::printf("creating vertexbuffer... ");

			IDirect3DIndexBuffer9 *ib;
			if (FAILED(device->CreateIndexBuffer(length, usage, format, pool, &ib, handle)))
			{
				throw core::FatalException("failed to create vertex buffer");
			}
			assert(NULL != ib);
			core::log::printf("done.\n");
			Attach(ib); // don't addref
		}

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
