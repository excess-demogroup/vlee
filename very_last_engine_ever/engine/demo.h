#pragma once

namespace engine
{
	class Demo
	{
	public:
		Demo(IDirect3DDevice9 *device) :
			device(device),
			done(false)
		{
			/* nothing special here, really */
		}

		virtual ~Demo() {}
		virtual void start() = 0;
		virtual void draw(float time) = 0;

		bool is_done()             { return done; }

	protected:
		IDirect3DDevice9 *device;
		bool done;
	};
}
