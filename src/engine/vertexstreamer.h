#pragma once

#include "../renderer/vertexbuffer.h"
#define VERTEX_STREAMER_VERTEX_BUFFER_SIZE (128*1024)

namespace engine
{
	class VertexStreamer
	{
	public:
		VertexStreamer(renderer::Device &device) :
			device(device),
			data(NULL)
		{
			vb = device.createVertexBuffer(sizeof(Vertex) * VERTEX_STREAMER_VERTEX_BUFFER_SIZE, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, Vertex::fvf);
		}
		
		int getPrimitiveOverflow()
		{
			switch (type)
			{
			case D3DPT_POINTLIST:    return 0;
			case D3DPT_LINELIST:     return index % 2;
			case D3DPT_TRIANGLELIST: return index % 3;
			case D3DPT_LINESTRIP:    return 0;
			default:
				assert(0);
				return 0;
			}
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
				case D3DPT_POINTLIST:    primitive_count = index;     break;
				case D3DPT_LINELIST:     primitive_count = index / 2; break;
				case D3DPT_TRIANGLELIST: primitive_count = index / 3; break;
				case D3DPT_LINESTRIP:    primitive_count = index - 1; break;
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
			assert(index < VERTEX_STREAMER_VERTEX_BUFFER_SIZE);
			assert(NULL != data);
			
			data->pos = pos;
			data++;
			
			// insert pos
			index++;
			
			if (
				(index >= VERTEX_STREAMER_VERTEX_BUFFER_SIZE - 2) && 
				(0 == getPrimitiveOverflow())
				)
			{
				end();
				begin(type);
			}
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
