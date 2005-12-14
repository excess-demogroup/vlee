#pragma once
#include <string>
#include <vector>
#include <map>
#include "SyncTimer.h"

/* todo
- editor
- testing
- different interpolators (linear, spline??, hermite)
*/

using namespace std;

struct SyncState
{
	SyncState() : row(0.0f), irow(0), drow(0.0f) {}
	float row;
	int irow;   //integer row
	float drow; //row fraction
	SyncTimer* timer;
	//inv: row = irow + drow;

	//bool running;
};

class SyncTrack;

typedef pair<string, SyncTrack> trackPair; 
typedef multimap<string, SyncTrack> trackMap;

class Sync
{
public:
	virtual SyncTrack& getTrack(string name, string group, int digits, bool signd);
	virtual void update();
	virtual void showEditor() {};
	virtual bool doEvents() { return true; };
	virtual void moveTo(int order) {};

	Sync(string filename, SyncTimer& timer); //filename is expected to contain %s twice for the name and group identifiers.
	virtual ~Sync(void); //autosaves
protected:
	SyncState state;
	SyncTimer& timer;

	string filename;

	trackMap tracks; //yeah, quite a silly datastructure. tis for the editor. the string is the group.
};