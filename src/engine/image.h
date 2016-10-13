#pragma once

#include "core/err.h"
#include "renderer/device.h"
#include "renderer/texture.h"

#include "engine/effect.h"
#include "engine/drawable.h"
#include "engine/mesh.h"

namespace engine {
	void drawQuad(renderer::Device &device, Effect *fx, float x, float y, float w, float h);
}
