#include ".\Sync.h"
#include ".\SyncTrack.h"
#include "uglyswitchhack.h"

SyncTrack& Sync::getTrack(string name, string group, int digits, bool signd)
{
	//check if it already exists. if so, return that.
	trackMap::_Pairii eq = tracks.equal_range(group);
	for(trackMap::iterator j = eq.first; j != eq.second; j++) {
		if(j->second.name() == name) return j->second;
	}

	//no track with specified name+group is present. create and load it.
	trackMap::iterator i = tracks.insert(trackPair(group, SyncTrack(&state,SyncTrackInfo(name,group,digits,signd))));
	i->second.loadData(filename);
	return i->second;
}

void Sync::update() {
	state.row = timer.getposition();
	state.irow = (int)state.row;
	state.drow = state.row-state.row;
}

Sync::Sync(string filename, SyncTimer& timer) : filename(filename), timer(timer)
{
	state.timer = &timer; //tracks.reserve(20);
}

Sync::~Sync(void)
{
}
