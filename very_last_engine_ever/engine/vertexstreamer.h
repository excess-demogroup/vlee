#pragma once

#include "../renderer/vertexbuffer.h"
#define VERTEX_STREAMER_VERTEX_BUFFER_SIZE 1024*2

namespace engine
{

	class VertexStreamer {
	public:
		VertexStreamer(renderer::Device &device) :
			device(device),
			data(NULL)
		{
			vb = device.createVertexBuffer(sizeof(Vertex) * VERTEX_STREAMER_VERTEX_BUFFER_SIZE, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, Vertex::fvf);
		}
		
		void begin(const D3DPRIMITIVETYPE type)
		{
			this->type = type;
			index = 0;
			data = (Vertex*)vb.lock(0, sizeof(Vertex) * VERTEX_STREAMER_VERTEX_BUFFER_SIZE, D3DLOCK_DISCARD);
			assert(NULL != data);
		}
		
		void end()
		{
			assert(NULL != data);
			vb.unlock();
			data = NULL;
			
			int primitive_count = 0;
			switch (type)
			{
				case D3DPT_POINTLIST:		primitive_count = index;			break;
				case D3DPT_LINELIST:		primitive_count = index / 2;		break;
				case D3DPT_LINESTRIP:		primitive_count = int(index) - 1;	break;
				case D3DPT_TRIANGLELIST:	primitive_count = index / 3;		break;
				case D3DPT_TRIANGLESTRIP:	primitive_count = int(index) - 2;	break;
				case D3DPT_TRIANGLEFAN:		primitive_count = int(index) - 2;	break;
				default: assert(0);
			}
			
			if (primitive_count > 0)
			{
				device->SetStreamSource(0, vb, 0, sizeof(Vertex));
				device->SetFVF(Vertex::fvf);
				device->DrawPrimitive(type, 0, primitive_count);
			}
		}

		void uv(const D3DXVECTOR2 &uv)
		{
			data->uv = uv;
		}

		void diffuse(const unsigned col)
		{
			data->diff = col;
		}

		void normal(const D3DXVECTOR3 &norm)
		{
			data->norm = norm;
		}

		void vertex(const D3DXVECTOR3 &pos)
		{
			if (index >= VERTEX_STREAMER_VERTEX_BUFFER_SIZE - 2)
			{
				end();
				begin(type);
			}
			
			assert(NULL != data);
			data->pos = pos;
			data++;
			
			// insert pos
			index++;
		}
		
	private:
		
		class Vertex
		{
		public:
			D3DXVECTOR3 pos;
			D3DXVECTOR3 norm;
			unsigned diff;
			D3DXVECTOR2 uv;
			static const int fvf = D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE | D3DFVF_TEX1;
		};

		renderer::Device &device;
		renderer::VertexBuffer vb;

		D3DPRIMITIVETYPE type;
		unsigned index;
		Vertex *data;
	};

}
