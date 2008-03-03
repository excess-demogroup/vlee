#ifndef VIDEO_H
#define VIDEO_H

#include "../renderer/texture.h"
namespace engine
{
	class VideoStream
	{
	public:
		int   getWidth();
		int   getHeight();
		void *nextFrame();
		
	private:
		unsigned int widht, height;
		unsigned int frames;
		
		unsigned current_frame;
		float fps;
		
		FILE *fp;
		unsigned start_offset;
		unsigned handle;
		
		void *buffer;
		
		static unsigned long currentHandle;
		unsigned long getHandle() { return currentHandle++; }
	};
	
//	Video loadVideo(const std::string filename);
	
	class VideoTexture : public renderer::Texture
	{
	public:
		VideoTexture(renderer::Device &device, UINT width, UINT height, UINT levels = 1, D3DFORMAT format = D3DFMT_A8R8G8B8,  D3DMULTISAMPLE_TYPE multisample = D3DMULTISAMPLE_NONE)
			: Texture(device, width, height, levels, 0, format)
		{
			
		}
	private:
		VideoStream videoDecoder;
	};
}

#endif /* VIDEO_H */
