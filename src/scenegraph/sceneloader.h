#pragma once

#include <string>
#include "../renderer/device.h"
#include "scene.h"

namespace scenegraph
{
	Scene *loadScene(renderer::Device &device, const std::string filename);
}
