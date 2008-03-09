#include "stdafx.h"
#include "sceneloader.h"

#include "../engine/mesh.h"

#include "node.h"
#include "prstransform.h"
#include "meshnode.h"
#include "target.h"

#include "../math/vector3.h"

#include "tinyxml.h"

using namespace scenegraph;
using math::Vector3;

float loadFloat(TiXmlElement *elem, const char *name, float default_val)
{
	const char *ret = elem->Attribute(name);
	if (!ret) return default_val;
	return float(atof(ret));
}

static std::string loadString(TiXmlElement *elem, const char *name, const std::string &defaultString = "")
{
	const char *ret = elem->Attribute(name);
	if (NULL == ret) return defaultString;
	return ret;
}

static math::Vector3 loadVector3(TiXmlNode* node)
{
	TiXmlElement *elem = node->ToElement();
	assert(NULL != elem);
	
	return math::Vector3(
		loadFloat(elem, "x", 0),
		loadFloat(elem, "y", 0),
		loadFloat(elem, "z", 0)
	);
}

static std::string getPathName(const std::string &fileName)
{
	std::string::size_type idx = fileName.rfind('/'); // find_last_of("\\/");
	if (std::string::npos != idx) return fileName.substr(0, idx + 1);
	else return "";
}

class SceneLoader
{
private:
	const std::string filename;
	const std::string basename;
	
public:
	SceneLoader(const std::string filename) :
		filename(filename),
		basename(getPathName(filename))
	{}

	Node *loadPrsTransform(renderer::Device &device, TiXmlElement *xmlElem)
	{
		PrsTransform *ret = new PrsTransform(loadString(xmlElem, "name"));
		
		ret->setPosition(Vector3(0, 0, 0));
		ret->setRotation(Vector3(0, 0, 0));
		ret->setScale(Vector3(1, 1, 1));
		
		TiXmlNode* curr = xmlElem->FirstChild();
		while (curr)
		{
			if (curr->Type() == TiXmlNode::ELEMENT)
			{
				TiXmlElement *currElem = curr->ToElement();
				assert(NULL != currElem);
				
				if      (strcmp(curr->Value(), "position") == 0) ret->setPosition(loadVector3(currElem));
				else if (strcmp(curr->Value(), "children") == 0) loadChildren(device, ret, currElem);
				else throw std::string("unknown element \"") + curr->Value() + std::string("\"");
			}
			curr = xmlElem->IterateChildren(curr);
		}
		
		return ret;
	}

	Node *loadMesh(renderer::Device &device, TiXmlElement *xmlElem)
	{
		std::string fileName = loadString(xmlElem, "file");
		engine::loadMesh(device, basename + fileName);
		
		MeshNode *ret = new scenegraph::MeshNode(loadString(xmlElem, "name"), NULL, NULL);
		return ret;
	}

	Node *loadTarget(TiXmlElement *xmlElem)
	{
		scenegraph::Target *ret = new scenegraph::Target(loadString(xmlElem, "name"));
		ret->setTarget(loadVector3(xmlElem));
		
		return ret;
	}

	void loadChildren(renderer::Device &device, Node *graphNode, TiXmlElement *xmlElem)
	{
		TiXmlNode* curr = xmlElem->FirstChild();
		while (curr)
		{
			if (curr->Type() == TiXmlNode::ELEMENT)
			{
				TiXmlElement *currElem = curr->ToElement();
				assert(NULL != currElem);
				
				Node *newChild = NULL;
				if      (strcmp(curr->Value(), "prs_transform") == 0) newChild = loadPrsTransform(device, currElem);
				else if (strcmp(curr->Value(), "mesh") == 0)          newChild = loadMesh(device, currElem);
				else if (strcmp(curr->Value(), "target") == 0)        newChild = loadTarget(currElem);
				else throw std::string("unknown element \"") + curr->Value() + std::string("\"");
				
				assert(NULL != newChild);
				graphNode->addChild(newChild);
			}
			curr = xmlElem->IterateChildren(curr);
		}
	}
};

Scene *scenegraph::loadScene(renderer::Device &device, const std::string filename)
{
	Scene *scene = new Scene(filename);
	TiXmlDocument doc;
	try
	{
		if (!doc.LoadFile(filename.c_str()))
		{
			if (doc.ErrorRow() > 0)
			{
				char temp[256];
				_snprintf(temp, 256, "error at line %d: %s", doc.ErrorRow(), doc.ErrorDesc());
				throw std::string(temp);
			}
			else throw std::string(doc.ErrorDesc());
		}
		
		SceneLoader sceneLoader(filename);
		
		TiXmlElement *xmlRoot = doc.RootElement();
		if (strcmp(xmlRoot->Value(), "scene") != 0) throw std::string("invalid root node \"") + xmlRoot->Value() + std::string("\"");
		
		TiXmlNode* curr = xmlRoot->FirstChild();
		while (curr)
		{
			if (curr->Type() == TiXmlNode::ELEMENT)
			{
				TiXmlElement *currElem = curr->ToElement();
				assert(NULL != currElem);
				
				if      (strcmp(curr->Value(), "children") == 0) sceneLoader.loadChildren(device, scene, currElem);
				else throw std::string("unknown element \"") + curr->Value() + std::string("\"");
			}
			curr = xmlRoot->IterateChildren(curr);
		}
		
		return scene;
	}
	catch (const std::string &str)
	{
		delete scene;
		throw core::FatalException("Failed to load scene \"" + filename + "\":\n"+ str);
	}
}
