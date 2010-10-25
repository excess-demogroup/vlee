#pragma once

#include "texture.h"
#include "core/comref.h"

namespace renderer {
	class VolumeTexture : public ComRef<IDirect3DVolumeTexture9> {
	public:
		explicit VolumeTexture(IDirect3DVolumeTexture9 *volumeTexture = NULL)
			: ComRef<IDirect3DVolumeTexture9>(volumeTexture)
		{
		}
		
		virtual ~VolumeTexture() {}

		D3DLOCKED_BOX lockBox(UINT level, const D3DBOX *box, DWORD flags = 0) {
			D3DLOCKED_BOX lockedBox;
			core::d3dErr(p->LockBox(
				level,
				&lockedBox,
				box,
				flags
				));
			return lockedBox;
		}

		void unlockBox(UINT level) {
			core::d3dErr(p->UnlockBox(level));
		}
	};
}
