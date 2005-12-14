#pragma once

#define VERTEX_STREAMER_VERTEX_BUFFER_SIZE 1024

class VertexStreamer {
public:
	VertexStreamer(IDirect3DDevice9 *device) : device(device), data(0), vb(device, sizeof(Vertex) * VERTEX_STREAMER_VERTEX_BUFFER_SIZE, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, Vertex::fvf) { }

	void begin(const D3DPRIMITIVETYPE type) {
		this->type = type;
		index = 0;
		data = (Vertex*)vb.lock(0, sizeof(Vertex) * VERTEX_STREAMER_VERTEX_BUFFER_SIZE, D3DLOCK_DISCARD);
	}

	void end() {
		assert(data);
		vb.unlock();
		data = 0;

		int primitive_count = 0;
		switch (type) {
			case D3DPT_POINTLIST:		primitive_count = index;			break;
			case D3DPT_LINELIST:		primitive_count = index / 2;		break;
			case D3DPT_LINESTRIP:		primitive_count = int(index) - 1;	break;
			case D3DPT_TRIANGLELIST:	primitive_count = index / 3;		break;
			case D3DPT_TRIANGLESTRIP:	primitive_count = int(index) - 2;	break;
			case D3DPT_TRIANGLEFAN:		primitive_count = int(index) - 2;	break;
			default: assert(0);
		}

		if (primitive_count > 0) {
			device->SetStreamSource(0, vb.get_vertex_buffer(), 0, sizeof(Vertex));
			device->SetFVF(Vertex::fvf);
			device->DrawPrimitive(type, 0, primitive_count);
		}
	}

	void uv(const D3DXVECTOR2 &uv) {
		data->uv = uv;
	}

	void diffuse(const unsigned col) {
		data->diff = col;
	}

	void normal(const D3DXVECTOR3 &norm) {
		data->norm = norm;
	}

	void vertex(const D3DXVECTOR3 &pos) {
		assert(data);
		if (index >= VERTEX_STREAMER_VERTEX_BUFFER_SIZE - 2) {
			end();
			begin(type);
		}

		data->pos = pos;
		data++;

		// insert pos
		index++;
	}

private:

	class Vertex {
	public:
		D3DXVECTOR3 pos;
		D3DXVECTOR3 norm;
		unsigned diff;
		D3DXVECTOR2 uv;
		static const int fvf = D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE | D3DFVF_TEX1;
	};

	IDirect3DDevice9 *device;

	core::VertexBuffer vb;

	D3DPRIMITIVETYPE type;
	unsigned index;
	Vertex *data;
};
