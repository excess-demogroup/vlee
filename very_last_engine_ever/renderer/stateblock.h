#pragma once

namespace renderer
{
	class StateBlock {
	public:
		StateBlock(IDirect3DStateBlock9 *stateblock = 0)
			: stateblock(stateblock)
		{
		}

	private:
	};
}
