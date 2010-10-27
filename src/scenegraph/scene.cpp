#include "stdafx.h"
#include "scene.h"

using namespace scenegraph;

static inline math::Vector3 lerp(std::map<float, math::Vector3>::const_iterator lower, std::map<float, math::Vector3>::const_iterator upper, float time)
{
	// find local time
	float t = (time - lower->first) / (upper->first - lower->first);
	
	// lerp
	math::Vector3 delta = upper->second - lower->second;
	return lower->second + delta * t;
}

static inline math::Quaternion lerp(std::map<float, math::Quaternion>::const_iterator lower, std::map<float, math::Quaternion>::const_iterator upper, float time)
{
	// find local time
	float t = (time - lower->first) / (upper->first - lower->first);

	// lerp
	return lower->second.slerp(upper->second, t);
}

math::Vector3 PrsAnim::getPos(float time)
{
	// find bounds
	std::map<float, math::Vector3>::const_iterator upper = posTrack.upper_bound(time);
	std::map<float, math::Vector3>::const_iterator lower = upper;
	lower--;

	// bounds check
	if (lower == posTrack.end()) return upper->second;
	if (upper == posTrack.end()) return lower->second;

	// interpolate
	return lerp(lower, upper, time);
}

math::Quaternion PrsAnim::getRot(float time)
{
	// find bounds
	std::map<float, math::Quaternion>::const_iterator upper = rotTrack.upper_bound(time);
	std::map<float, math::Quaternion>::const_iterator lower = upper;
	lower--;

	// bounds check
	if (lower == rotTrack.end()) return upper->second;
	if (upper == rotTrack.end()) return lower->second;

	// interpolate
	return lerp(lower, upper, time);
}

math::Vector3 PrsAnim::getScale(float time)
{
	// find bounds
	std::map<float, math::Vector3>::const_iterator upper = scaleTrack.upper_bound(time);
	std::map<float, math::Vector3>::const_iterator lower = upper;
	lower--;

	// bounds check
	if (lower == scaleTrack.end()) return upper->second;
	if (upper == scaleTrack.end()) return lower->second;

	// interpolate
	return lerp(lower, upper, time);
}

void Scene::anim(float time)
{
	std::map<PrsTransform*, PrsAnim>::iterator i;
	
	for (i = animTracks.begin(); i != animTracks.end(); ++i)
	{
		PrsTransform *target = i->first;
		target->setPosition(i->second.getPos(time));
		target->setRotation(i->second.getRot(time));
		target->setScale(i->second.getScale(time));
	}
}
