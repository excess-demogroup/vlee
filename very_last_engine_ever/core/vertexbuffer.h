#pragma once

namespace core
{
	class VertexBuffer
	{
	public:
		VertexBuffer(IDirect3DVertexBuffer9 *vb = 0) : vb(vb) {}

		VertexBuffer(IDirect3DDevice9 *device, UINT length, DWORD usage, DWORD fvf, D3DPOOL pool = D3DPOOL_DEFAULT, HANDLE* handle = 0) : vb(0) {
			assert(0 != device);
			core::log::printf("creating vertexbuffer... ");
			if (FAILED(device->CreateVertexBuffer(length, usage, fvf, pool, &vb, handle)))
				throw std::exception("failed to create vertex buffer");
			core::log::printf("done.\n");
		}

		~VertexBuffer() {
			if (vb) vb->Release();
		}

		void *lock(UINT offset, UINT size, DWORD flags) {
			void *mem = 0;
			vb->Lock(offset, size, &mem, flags);
			return mem;
		}

		void unlock() {
			vb->Unlock();
		}

		IDirect3DVertexBuffer9 *get_vertex_buffer() const {
			return vb;
		}

	private:
		IDirect3DVertexBuffer9 *vb;
	};
}
