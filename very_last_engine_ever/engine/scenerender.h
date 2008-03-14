#pragma once

#include "../scenegraph/scene.h"
#include "../scenegraph/camera.h"
#include "../math/matrix4x4.h"

namespace engine
{

	class SceneRenderer
	{
	public:
		SceneRenderer(scenegraph::Scene *scene, scenegraph::Camera *camera) : scene(scene), camera(camera)
		{

		}

		void visit(scenegraph::Node *node, math::Matrix4x4 world);
		void draw();

	private:
		scenegraph::Scene *scene;
		scenegraph::Camera *camera;
	public:
		math::Matrix4x4 view, projection;
	};

}
