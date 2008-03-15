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
	switch (node->getType())
	{
	case NODE_TRANSFORM:
		world = reinterpret_cast<Transform*>(node)->getTransform() * world;
		break;

	case NODE_DRAWABLE:
		{
			MeshNode *mesh = reinterpret_cast<MeshNode*>(node);
			if (NULL != mesh->effect)
			{
				mesh->effect->setMatrices(world, view, projection);
				mesh->effect->commitChanges();
			}
			mesh->draw();
		}
		break;

	default: break;
	}
	
	for (Node::child_iterator i = node->beginChildren(); i != node->endChildren(); ++i) visit(*i, world);
}

void SceneRenderer::draw()
{
	printf("\n");
	visit(scene, math::Matrix4x4::identity());
}

