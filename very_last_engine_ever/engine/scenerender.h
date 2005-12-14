#pragma once

class SceneRenderer {
public:
	SceneRenderer(scenegraph::Scene *scene, scenegraph::Camera *camera) : scene(scene), camera(camera) {

	}

	void draw();

private:
	scenegraph::Scene *scene;
	scenegraph::Camera *camera;
};
