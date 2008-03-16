#include "stdafx.h"
#include "scenerender.h"

#include "../scenegraph/transform.h"
#include "../scenegraph/drawable.h"
#include "../scenegraph/meshnode.h"

#include <stack>

using namespace engine;
using namespace scenegraph;

void SceneRenderer::visit(Node *node, math::Matrix4x4 world)
{
	if (NODE_DRAWABLE == node->getType())
	{
		MeshNode *mesh = reinterpret_cast<MeshNode*>(node);
		if (NULL != mesh->effect)
		{
			mesh->effect->setMatrices(node->getAbsoluteTransform(), view, projection);
			mesh->effect->commitChanges();
		}
		mesh->draw();
	}
	
	for (Node::child_iterator i = node->beginChildren(); i != node->endChildren(); ++i) visit(*i, world);
}

void SceneRenderer::draw()
{
	visit(scene, math::Matrix4x4::identity());
}

