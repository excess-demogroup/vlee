#pragma once
#include "Sync.h"

struct SyncTrackInfo {
	SyncTrackInfo(string name, string group, int digits, bool signd) : name(name), group(group), digits(digits), signd(signd) {};
	SyncTrackInfo();
	string name;
	string group;
	int digits;
	bool signd;
};

struct SyncTrackItem {
	SyncTrackItem(): value(0), flag(false) {};
	SyncTrackItem(int value, bool flag): value(value), flag(flag) {};
	int value;
	bool flag; //changes track colour in gui; usually denotes that interpolation between current and next item is on.
};

typedef map<int, SyncTrackItem> SyncTrackData;

struct SyncBoundsData {
	int lower_pos;
	int lower_value;
	int upper_pos;
	int upper_value;
	bool flag;
	inline float linear(float s) {
		return ((float)(s - lower_pos) / (float)(upper_pos - lower_pos) * (float)(upper_value - lower_value) + (float)lower_value);
	}
};

class SyncTrack
{
public:
	SyncTrack(SyncState* state, SyncTrackInfo info);
	~SyncTrack(void);
	void loadData(string filename); //attempts to load data from file.
	void saveData(); //saves data to file.
	string name() { return info.name; };
	int digits() { return info.digits; };
	bool signd() { return info.signd; };

	//auto-interpolating current value getters
	float getFloatValue();
	int getIntValue();
	bool getTrig();

	bool visible;

	//hardcore l33t diy-versions
	SyncBoundsData getBoundsData();
	bool getFlag();

	SyncTrackData::iterator lower(int i) { return data.lower_bound(i); };

protected:
	void validateAux();
	void shiftData(int start, int d);

	SyncState* state; //reference to state (current row, etc) owned by the Sync that owns this.
	int auxirow;

	SyncBoundsData auxbounds;
	SyncTrackInfo info;
	string filename;

	int lasttrigrow; //the row number on which the latest yet unprocessed nonzero value was encountered

	SyncTrackData data;
	SyncTrackData copybuffer;
	int copybufferlen;
	pair<SyncTrackData::iterator, SyncTrackData::iterator> range; //iterators pointing to 
		//the entry directly before or at the current row when we last checked;
		//the entry directly after the current row when we last checked;

	//inv: auxrow <= state.irow
	//inv: range.first->first <= auxrow < range.second->first
	//inv: ++(range.first) == range.second
friend class SyncEditor;

};
