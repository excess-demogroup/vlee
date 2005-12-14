#define _CRT_SECURE_NO_DEPRECATE
#include ".\synctrack.h"
#include <windows.h>
#include <stdio.h>
#include <assert.h>
#include <sstream>
#include "uglyswitchhack.h"

SyncTrack::SyncTrack(SyncState* state, SyncTrackInfo info) : state(state), info(info), auxirow(INT_MIN), visible(true)
{

}

SyncTrack::~SyncTrack(void)
{
	if(!filename.empty()) this->saveData();
}

//attempts to load data as stored in the file.
void SyncTrack::loadData(string _filename)
{
	char fn[256];
	filename = _filename;
	wsprintf(fn,filename.c_str(),info.group.c_str(),info.name.c_str());
	filename = fn;
	FILE* fp = fopen(fn,"rb");
	if(fp) {
		pair<int, SyncTrackItem> d;

		SyncTrackData::iterator i = data.begin();

		while(!feof(fp)) {
			if(fread(&d,sizeof(d),1,fp)) 
				i = data.insert(i,d);
		}
		fclose(fp);
	}

	{	//add infinity on ends, removing the need for boundschecking.
		SyncTrackData::iterator data_min = data.begin();
		if((data.empty()) || (data_min->first != INT_MIN)) 
			data.insert(pair<int, SyncTrackItem>(INT_MIN, SyncTrackItem()));

		SyncTrackData::reverse_iterator data_max = data.rbegin();
		if(data_max->first != INT_MAX) 
			data.insert(pair<int, SyncTrackItem>(INT_MAX, SyncTrackItem()));
	}

	validateAux(); //init range before the demo runs.
}

void SyncTrack::saveData()
{
	if(!data.empty()) {
		char fn[256];
		wsprintf(fn,filename.c_str(),info.group,info.name);
		FILE* fp = fopen(fn,"wb");
		SyncTrackData::iterator i = data.begin();
		for(;i!=data.end();i++) {
			fwrite(&(*i),sizeof(pair<int, SyncTrackItem>),1,fp);
		}
		fclose(fp);
	}
}


float SyncTrack::getFloatValue() {
	validateAux();
	if(auxbounds.flag) {
		return auxbounds.linear(state->row); 
	} else {
		return (float)auxbounds.lower_value;
	}
}

int SyncTrack::getIntValue() {
	validateAux();
	if(auxbounds.flag) {
		return (int)auxbounds.linear(state->row); 
	} else {
		return auxbounds.lower_value;
	}
}

bool SyncTrack::getTrig() {
	validateAux();
	if((auxbounds.lower_pos == auxirow) && (lasttrigrow != auxirow)) { //lower_pos == irow means there is data on the current row.
		lasttrigrow = auxirow;
		return true;
	} else {
		return false;
	}
}

SyncBoundsData SyncTrack::getBoundsData() {
	validateAux();
	return auxbounds;
}

bool SyncTrack::getFlag() {
	validateAux();
	return auxbounds.flag;
}

//pre: d!=0
//CRAP!
void SyncTrack::shiftData(int start, int d) {
	if(d>0) {
		SyncTrackData::reverse_iterator i;
		for(SyncTrackData::reverse_iterator j = data.rbegin(); j->first > start;) {
			data[j->first+d] = j->second;
			i = j; i++;
			data.erase(j->first);
			j = i;
		}
	} else { //j<0
		SyncTrackData::iterator i;
		for(SyncTrackData::iterator j = data.lower_bound(start); j!=data.end();) {
			data[j->first+d] = j->second;
			i = j; i++;
			data.erase(j->first);
			j = i;
		}
	}
}

//pre: data.first.key == -inf && data.last.key == inf. { ==> (2 <= data.size()) }
//pre: INT_MIN < state->irow < INT_MAX (dah)
//pre: range ~ auxbounds. (values and positions and flag in range (the real data) and the auxbounds struct are equal)
//post: pre && auxrow == state->irow && auxbounds.lower_pos < state->irow <= auxbounds.upper_pos (and values match these).
void SyncTrack::validateAux() {
	if(auxirow != state->irow) { //row has been invalidated.
		auxirow = state->irow;

		if(state->timer->isrunning()) { //'linear' search in case of linearly increasing time and correct previous state
			// almost constant time when running, assuming we hardly move more than one 'step' at a time. 
			//(lower := upper, upper := upper++)
			while((range.second != data.end()) && (range.second->first <= auxirow)) { //inv: ++(range.first) == range.second.
				++(range.second);
				++(range.first);
			} //in case of (the usual) more than 1 frame per row, this loop will only run once.
			//state->running shouldn't be true if range.second can become data.end(), but below we're returning the correct 
			//values (but slower) anyway. just in case.
		}
		if((!state->timer->isrunning()) || (range.second==data.end())) { //no linear increase for you mister!
			range.first = data.lower_bound(auxirow);			//auxirow <= range.first.key
			if(range.first->first > auxirow) --(range.first);	//range.first.key <= auxirow < (++(range.first)).key
			range.second = range.first; ++(range.second);		//range.first.key <= auxirow < range.second.key.
		}
		auxbounds.lower_pos   = range.first->first;
		auxbounds.lower_value = range.first->second.value;
		auxbounds.flag        = range.first->second.flag;
		auxbounds.upper_pos   = range.second->first;
		auxbounds.upper_value = range.second->second.value;
	}
}

/*
//post: auxrow == state->irow && auxbounds.lower_pos < state->irow <= auxbounds.upper_pos (and values match these).
void SyncTrack::validateAux() {
	if(auxirow != state->irow) { //row has been invalidated.
		auxirow = state->irow;

		if(auxbounds.upper_pos <= auxirow) { //bounds have been invalidated too, meaning we're 'on' or past a new value.
			if(data.empty()) {
				auxbounds.lower_pos   = INT_MIN;
				auxbounds.lower_value = 0;
				auxbounds.flag        = false;

				auxbounds.upper_pos   = INT_MAX; //inf!!
				auxbounds.upper_value = 0;
			} else {

				bool beforebegin = false;
				if(state->running && (range.first!=data.end())) { //'linear' search in case of linear time and correct previous state
					// almost constant time when running, assuming we hardly move more than one 'step' at a time. (lower := upper, upper := upper++)
					while((range.second != data.end()) && (range.second->first <= auxirow)) { //inv: ++(range.first) == range.second.
						++(range.second);
						++(range.first);
					} //in case of (the usual) more than 1 frame per row, this loop will only run once.

				} else { //no guarantee we're currenly close to where we moved, better go for O(log N) time than the chance for a crash or O(N).
					range = data.equal_range(auxirow);
					if(range.first == range.second) { //key doesn't exactly exist, so it picks the lowest key after it twice.
						if(range.first == data.begin()) { //auxrow < the lowest key
							beforebegin = true;
							range.first = data.end(); //invalidate iterator (ugly!!)
						} else {
							range.first--;
						}
					}
				}
				//assert(range.first->first <= auxirow && auxirow < range.second->first); 

				//copy range data to auxbounds for speed readability and sense.
		
				if(range.first == data.end()) { //range.first is invalid
					auxbounds.lower_pos   = INT_MIN;
					auxbounds.lower_value = 0;
					auxbounds.flag        = false;
				} else {
					auxbounds.lower_pos   = range.first->first;
					auxbounds.lower_value = range.first->second.value;
					auxbounds.flag        = range.first->second.flag;
				}
				if(range.second==data.end()) { //hooray for STL's "past-the-end" notion. here it actually makes sense.
					auxbounds.upper_pos   = INT_MAX; //inf!!
					auxbounds.upper_value = auxbounds.lower_value;
				} else {
					auxbounds.upper_pos   = range.second->first;
					auxbounds.upper_value = range.second->second.value;
				}
			}
		}
	}
}
*/