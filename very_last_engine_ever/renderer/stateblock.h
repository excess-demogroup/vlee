#pragma once

namespace renderer
{
	class StateBlock {
	public:
		StateBlock(IDirect3DStateBlock9 *stateblock = 0) : stateblock(stateblock) {}

		StateBlock(IDirect3DDevice9 *device, D3DSTATEBLOCKTYPE type) : stateblock(0) {
			assert(0 != device);
			device->CreateStateBlock(type, &stateblock);
			if (!stateblock) throw FatalException("failed to create vertex buffer");
		}

		~StateBlock() {
			if (stateblock) stateblock->Release();
		}

		IDirect3DStateBlock9 *getStateBlock() const {
			return stateblock;
		}

	private:
		IDirect3DStateBlock9 *stateblock;
	};
}
