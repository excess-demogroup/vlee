#pragma once

#include "../renderer/device.h"

namespace engine
{
	class Demo
	{
	public:
		Demo(Device &device, Surface &backbuffer) :
			device(device),
			backbuffer(backbuffer),
			done(false)
		{
			/* nothing special here, really */
		}

		virtual ~Demo() {}
		virtual void start() = 0;
		virtual void draw(float time) = 0;

		bool is_done()             { return done; }

	protected:
		Device  &device;
		Surface &backbuffer;
		bool done;
	};
}
