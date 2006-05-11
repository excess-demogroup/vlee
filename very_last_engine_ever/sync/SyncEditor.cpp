#include ".\synceditor.h"
#include <windows.h>
#include <sstream>
#include "uglyswitchhack.h"

#define ALT_PRESSED (LEFT_ALT_PRESSED|RIGHT_ALT_PRESSED)
#define CTRL_PRESSED (LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED)
//SHIFT_PRESSED already defined

void SyncEditor::update()
{
	if(state.timer->isrunning()) state.row = timer.getposition();
	state.irow = (int)state.row;
	state.drow = state.row-state.row;

	if(curpos != state.irow)
	{
		curpos = state.irow;
		paint();
	}
}

//sortof the reverse of update().
void SyncEditor::forceposition() {
	if(curpos!=state.irow) {
		state.row = (float)curpos;
		state.irow = curpos;
		state.drow = 0.0f;
		timer.setposition(curpos);
	}
}

void SyncEditor::paint()
{
	//make sure the current track iterator is valid;
	if(curtrack_iter==tracks.end()) {
		curtrack_iter = tracks.begin(); 
		for(int i=0;i<curtrack;i++) ++curtrack_iter;
	}

	while (curpos < patternoffsets[curptn]) curptn--;
	while ((patternoffsets.size() > unsigned(curptn + 1)) && (patternoffsets[curptn + 1] < curpos)) curptn++;

	con.cls();
	string prevgroup = "";
	int trackx = 0;


	con.offset(0,3);
	int ht = HT - con.yoff;
	int hht = ht/2 -1;
	int firstpos = curpos - hht;
	int firstptn = curptn - hht + 1;
	short col, bg;

	ostringstream oss;

	{ //draw bars
		con.fill(0,-1,colour(0x111),(char)0304,WD);
		con.fill(0,hht,colour(0,0x111),' ',WD);
		//con.fillvertical(2,0,colour(0x111),(char)179,ht);
		bg = (orderfocus && editing) ? 0x001 : 0x010;
		con.fillvertical(0,-3,colour(0,bg),' ',HT);
		con.fillvertical(1,-3,colour(0,bg),' ',HT);
		con.fillvertical(2,-3,colour(0x222),(char)221,HT);
		//con.fill(2,-1,colour(0x111),(char)194,1);
		con.fill(2,hht,colour(0x222,0x111),(char)221,1);
	}

	{ //draw patternoffsets
		int ptn = firstptn;
		int lastptnoff = 0;

		int ptnoffsize = (int)patternoffsets.size();
		for(int i = 0; i < ht; i++, ptn++) {
			//set bg + fg
			if(ptn==curptn+1) {
				if(orderfocus) {
					bg = 0x010; col = 0x000;
				} else {
					bg = 0x111; col = 0x000;
				}
			} else {
				bg = 0x010; col = 0x000;
			}
			if(editing) bg = 0x001;

			//set data and draw	
			if((ptn>0) && (ptn<ptnoffsize)) {
				char buf[10];
				int value = patternoffsets[ptn]-lastptnoff;
				if(editing) value = (int)entry;
				wsprintf(buf,"%x",value);
				con.put(trackx,i,colour(col,bg),buf,'0',2); //&&pos<ptnlen
				lastptnoff = patternoffsets[ptn];
			}
		}
	}

	int pos = firstpos;
	//draw row numbers
	for(int i = 0; i < ht; i++, pos++) {
		bg=0x000;
		if(!((pos-patternoffsets[curptn])%8)) col = 0x222; else col = 0x111;
		if(pos==curpos) { col=0x000; bg=0x111; }
		//bool endnumbers = (orderfocus) ? (pos<(int)patternoffsets.size()-1) : ;//FIXME!!
		if (pos >= patternoffsets[curptn] && ((patternoffsets.size() > unsigned(curptn + 1)) && pos < patternoffsets[curptn+1])) con.put(3,i,colour(col,bg),pos-patternoffsets[curptn],2);
	}
	//draw tracks.
	con.offset(6,3);

	int track = 0;
	//int mintrack = tracks.size()
	for(trackMap::iterator ti = tracks.begin(); ti!=tracks.end(); ti++, track++) if(5>abs(curtrack-track)) {
		//draw group and track name
		SyncTrack& t = ti->second;
		if(ti->first != prevgroup) {
			trackx--;
			con.fillvertical(trackx,-con.yoff,colour(0x111),(char)179,HT);
			//con.fill(trackx,-1,colour(0x111),(char)197,1);
			con.fill(trackx,hht,colour(0,0x111),' '/*(char)222*/,1);
			trackx++;
			con.put(trackx,-con.yoff,colour(0x222),ti->first);
			prevgroup = ti->first;
		}
		con.put(trackx,1-con.yoff,colour(0x111),t.name());

		//draw track data.
		
		//state.running = true;

		int pos = firstpos;
		//SyncBoundsData bounds = {INT_MIN, 0, patternoffsets[curptn], 0, false}; //FIXME: nodig? correct?
		SyncTrackData::iterator upper = t.data.upper_bound(pos-1);
		SyncTrackData::iterator lower = upper; --lower;
		for(int i = 0; i < ht; i++, pos++) {
			//set bg + fg
			col = 0x111;
			bg = 0x000;

			//set data
			if(upper->first <= pos) {
				//state.row = (float)(state.irow = pos);
				//bounds = t.getBoundsData();
				lower++;
				upper++;
			}

			int value = lower->second.value;
			bool editinghere = false;

			if(selection && (selstartrow <= pos) && (pos <= selendrow) && (selstarttrack <= track) && (track <= selendtrack)) {
				bg = 0x001;
			}
			if(selecting && (selstartrow <= pos) && (pos <= curpos) && (selstarttrack <= track) && (track <= curtrack)) {
				bg = 0x011;
			}

			if(lower->second.flag) col = 0x100;
			if(!((pos-patternoffsets[curptn])%8)) col*=2;// = 0x222;

			//set colour
			if(pos==curpos) {
				if((!orderfocus) && (track==curtrack)) {
					if(editing) {
						value = (int)entry;
						editinghere = true;
					}
				} else {
					col = 0x000;
					bg = 0x111;
				}
			}

			if (pos >= patternoffsets[curptn] && (patternoffsets.size() > unsigned(curptn + 1) && pos < patternoffsets[curptn+1])) { //&&pos<ptnlen
				if((lower->first == pos) || (editinghere)) {
					if((t.digits()==8) && !t.signd()) { //32bit unsigned quickhack. (stored as signed int anyway)
						con.put(trackx, i, colour(col,bg), (unsigned int)value, t.digits());
					} else {
						con.put(trackx, i, colour(col,bg), value, t.signd(), t.digits());
					}
				} else {
					if(t.signd()) {
						con.put(trackx, i, colour(col,bg), " ");
						con.put(trackx+1, i, colour(col,bg), "",'-',t.digits()); 
					} else {
						con.put(trackx, i, colour(col,bg), "",'-',t.digits());
					}
				}
			}
		}

		trackx += t.digits() + (t.signd()?2:1);
		//state.running = runningstate;
		//state.row = (float)(state.irow = 0);
		t.validateAux();
	}

	if(!orderfocus && editing) {
		con.offset(0,0);
		con.fillcolour(2,0,colour(0x222,0x001),WD-2);
		con.fillcolour(3,HT-1,colour(0x111,0x001),WD-2);
		con.fillcolour(2,HT-1,colour(0x222,0x001),1);
	}
	con.flip();
}

SyncEditor::SyncEditor(string filename, SyncTimer& timer): 
	Sync(filename, timer), 
	curpos(0), 
	curptn(0), 
	curtrack(0), 
	orderfocus(false), 
	editing(false), 
	curtrack_iter(tracks.begin()),
	editoropen(false),
	entry(0),
	selecting(false),
	selection(false),
	visible(false)
{
	FILE* fp = fopen("data\\pattern_order.sync", "rb");
	if (fp)
	{
		int d;
		while (!feof(fp))
		{
			if (fread(&d,sizeof(int),1,fp)) patternoffsets.push_back(d);
		}
		fclose(fp);
	}
	else
	{
		patternoffsets.push_back(0x0);
		patternoffsets.push_back(0x80);
		//patternoffsets.push_back(0xc0);
	}
}

SyncEditor::~SyncEditor(void)
{
	saveData();
}

void SyncEditor::saveData() {
	FILE* fp = fopen("data\\pattern_order.sync","wb");
	vector<int>::iterator i = patternoffsets.begin();
	fwrite(&(*i), sizeof(int), patternoffsets.size(), fp);
	fclose(fp);
	for (trackMap::iterator i=tracks.begin();i!=tracks.end();i++) i->second.saveData();
}

void SyncEditor::showEditor()
{
	visible = true;
	editoropen = true;
	con.open();
	paint();
}

bool SyncEditor::doEvents()
{
	if (visible) return con.doevents(*this);
	else return true;
}

bool SyncEditor::mouseevent(MOUSE_EVENT_RECORD e)
{
	return true;
}

bool SyncEditor::resizeevent(WINDOW_BUFFER_SIZE_RECORD e)
{
	return true;
}

void SyncEditor::unedit()
{
	if (editing)
	{
		editing = false;
		SyncTrackData::iterator i = curtrack_iter->second.data.upper_bound(curpos); --i;
		curtrack_iter->second.data[curpos] = SyncTrackItem((int)entry,i->second.flag);
		//curtrack_iter->second.data.insert(pair<int, SyncTrackItem>(curpos, SyncTrackItem((int)entry,false))); //FIXME: false --> proper flag
	}
}

void SyncEditor::applyorderchange() {
	int d = (int)entry - (patternoffsets[curptn+1]-patternoffsets[curptn]);
	if (d)
	{
		for (unsigned i=curptn+1;i<patternoffsets.size();i++)
		{
			patternoffsets[i]+=d;
		}

		for (trackMap::iterator i=tracks.begin();i!=tracks.end();i++)
		{
			if (d>0)
			{
				//i->second.shiftData(patternoffsets[curptn+1],d);
			}
			else
			{
				//i->second.shiftData(patternoffsets[curptn+1],d);
			}
		}
	}
}

void SyncEditor::moveTo(int order)
{
	curptn = order;
	curpos=patternoffsets[curptn];
	forceposition();
	paint();
}

bool SyncEditor::keyevent(KEY_EVENT_RECORD e)
{
	if(e.bKeyDown)
	{	
		switch(e.wVirtualKeyCode)
		{
			ifcase(VK_TAB)
			{
				orderfocus = !orderfocus;
				paint();
				return true;
			}
			elsecase(VK_SPACE)
			{
				//state.running = !state.running;
				unedit();
				paint();
				if(timer.isrunning()) {
					timer.pause();
				} else {
					saveData();
					timer.play();
				}
			}
		}

		if (orderfocus)
		{
			switch(e.wVirtualKeyCode)
			{
				ifcase(VK_UP)
				{
					if (curptn>0)
					{
						--curptn;
						curpos=patternoffsets[curptn];
						forceposition();
						paint();
					}

				}
				elsecase(VK_DOWN)
				{
					if (curptn<(int)(patternoffsets.size()-2))
					{ //last patternoffset is bogus.
						++curptn;
						curpos=patternoffsets[curptn];
						forceposition();
						paint();
					}

				}
				elsecase(VK_RETURN)
				{
					editing = !editing;
					if (!editing) applyorderchange();
					else entry = patternoffsets[curptn+1]-patternoffsets[curptn];
					paint();
				}
				elsecase(VK_INSERT)
				{
					//patternoffsets.resize(patternoffsets.size()+1);
					for (int i = int(patternoffsets.size()) - 1; i > curptn + 1; i--)
					{
						patternoffsets[i] += 0x80;
					}
					vector<int>::iterator j = patternoffsets.begin();
					j+=curptn+2;
					patternoffsets.insert(j,patternoffsets[curptn+1]+0x80);
					paint();
				}
				elsecase(VK_DELETE)
				{
					int d = patternoffsets[curptn+1]-patternoffsets[curptn];
					vector<int>::iterator j = patternoffsets.begin();
					j+=curptn + 1;
					patternoffsets.erase(j);
					for (int i = curptn+1; i < int(patternoffsets.size()); i++)
					{
						patternoffsets[i] -= d;
					}
					paint();
				}
			}

			if(editing) {
				if('0' <= e.wVirtualKeyCode && e.wVirtualKeyCode <= 'F') {
					//editing == true
					entry *= 0x10;
					entry += (e.wVirtualKeyCode<='9')? e.wVirtualKeyCode-'0' : e.wVirtualKeyCode-'A' + 0xA;
					entry &= 0xFF;
					paint();
				}
			}
		} else { //pattern focus
			if(e.dwControlKeyState&SHIFT_PRESSED) { //shift!
				switch(e.wVirtualKeyCode) {
					ifcase(VK_UP) {
						//unedit();
						SyncTrackData::iterator i = curtrack_iter->second.data.find(curpos);
						if(i!=curtrack_iter->second.data.end()) {
							i->second.value += (e.dwControlKeyState&CTRL_PRESSED) ? 0x10 : 1;
							paint();
						}

					} elsecase(VK_DOWN) {
						//unedit();
						SyncTrackData::iterator i = curtrack_iter->second.data.find(curpos);
						if(i!=curtrack_iter->second.data.end()) {
							i->second.value -= (e.dwControlKeyState&CTRL_PRESSED) ? 0x10 : 1;
							paint();
						}

					} elsecase(VK_LEFT) {
						unedit();
						--curtrack_iter;--curtrack;
						if(curtrack_iter==tracks.end()) { curtrack_iter--;curtrack=(int)tracks.size()-1; }
						string g = curtrack_iter->first;

						do {
							--curtrack_iter;--curtrack;
							if(curtrack_iter==tracks.end()) { curtrack_iter--;curtrack=(int)tracks.size()-1;	}
						} while(g==curtrack_iter->first);

						++curtrack_iter;++curtrack;
						if(curtrack_iter==tracks.end()) { curtrack_iter=tracks.begin();curtrack=0; }
						paint();

					} elsecase(VK_RIGHT) {
						unedit();
						string g = curtrack_iter->first;
						while(g==curtrack_iter->first) {
							++curtrack_iter;
							++curtrack;
							if(curtrack_iter==tracks.end()) {
								curtrack_iter=tracks.begin();
								curtrack=0;
							}
						}
						paint();
					}

				}
			} else { //no shift!
				if(e.dwControlKeyState&CTRL_PRESSED) { //ctrl!
					switch(e.wVirtualKeyCode) {
						ifcase('X') {
						} elsecase('C') {
							trackMap::iterator t = tracks.begin();
							int track = 0;
							for(;t!=tracks.end();t++,track++) {
								if((selstarttrack <= track) && (track <= selendtrack)) {
									t->second.copybuffer.clear();
									SyncTrackData::iterator i = t->second.data.lower_bound(selstartrow);
									for(;i!=t->second.data.upper_bound(selendrow);i++) {
										t->second.copybuffer[i->first - selstartrow] = i->second;
									}
								}
								t->second.copybufferlen = selendrow - selstartrow + 1;
							}
						} elsecase('V') {
							trackMap::iterator t = tracks.begin();
							int track = 0;
							for(;t!=tracks.end();t++,track++) {
								
								
								if((selstarttrack <= track) && (track <= selendtrack)) {
									//erase stuff
									SyncTrackData::iterator l = t->second.data.lower_bound(curpos);
									SyncTrackData::iterator u = t->second.data.upper_bound(curpos+t->second.copybufferlen);
									t->second.data.erase(l,u);//curpos,curpos+t->second.copybufferlen);
									//write stuff
									SyncTrackData::iterator i = t->second.copybuffer.lower_bound(0);
									for(;i!=t->second.copybuffer.upper_bound(t->second.copybufferlen);i++) {
										t->second.data[i->first + curpos] = i->second;
									}
								}
							}
							paint();
						}
					}
				} else { //no ctrl, no shift!
					switch(e.wVirtualKeyCode) {
						ifcase(VK_UP) {
							unedit();
							--curpos;
							if(curpos<patternoffsets[curptn]) curpos = patternoffsets[curptn+1] - 1;
							forceposition();
							paint();

						} elsecase(VK_DOWN) {
							unedit();
							++curpos;
							if(curpos>=patternoffsets[curptn+1]) curpos = patternoffsets[curptn];
							forceposition();
							paint();

						} elsecase(VK_PRIOR) {
							unedit();
							curpos-=0x10;
							if(curpos<patternoffsets[curptn]) curpos = patternoffsets[curptn];
							forceposition();
							paint();

						} elsecase(VK_NEXT) {
							unedit();
							curpos+=0x10;
							if(curpos>=patternoffsets[curptn+1]) curpos = patternoffsets[curptn+1]-1;
							forceposition();
							paint();

						} elsecase(VK_LEFT) {
							unedit();
							--curtrack;
							--curtrack_iter;
							if(curtrack<0) {
								curtrack=(int)tracks.size()-1;
								curtrack_iter=tracks.end(); --curtrack_iter;
							}
							paint();

						} elsecase(VK_RIGHT) {
							unedit();
							++curtrack;
							++curtrack_iter;
							if(curtrack>=(int)tracks.size()) {
								curtrack=0;
								curtrack_iter=tracks.begin();
							}
							paint();

						} elsecase(VK_RETURN) {
							state.irow = -31337666;
							unedit();
							paint();

						} elsecase(VK_DELETE) {
							if(editing) {
								editing = false; //cancel entry of a new number
							} else {
								curtrack_iter->second.data.erase(curpos);
							}
							paint();

						} elsecase(VK_OEM_MINUS) {
							if(curtrack_iter->second.signd()) {
								if(editing) entry = -entry; else {
									SyncTrackData::iterator i = curtrack_iter->second.data.find(curpos);
									if(i!=curtrack_iter->second.data.end()) i->second.value = -i->second.value;
								}
							}
							paint();

						} elsecase('I') {
							SyncTrackData::iterator i = curtrack_iter->second.data.upper_bound(curpos); //gets first value *after* the current position
							--i; //now i is last value before or on the current position.
							i->second.flag = !i->second.flag;
							state.irow = -31337666;
							paint();

						} elsecase('Z') {
							if(selecting) {
								selecting = false;
								selection = false;
							} else {
								selecting = true;
								selstarttrack = curtrack;
								selstartrow = curpos;
							}
							paint();
						} elsecase('X') {
							if(selecting) {
								selendtrack = curtrack;
								selendrow = curpos;
								selection = true;
								selecting = false;
							}
							paint();
						}
					}
					if('0' <= e.wVirtualKeyCode && e.wVirtualKeyCode <= 'F') {
						if(!editing) {
							editing = true;
							entry = 0;
						}
						//editing == true
						entry *= 0x10;
						entry += (e.wVirtualKeyCode<='9')? e.wVirtualKeyCode-'0' : e.wVirtualKeyCode-'A' + 0xA;
						paint();
					}
				}
			}
		}
		if(e.dwControlKeyState&ALT_PRESSED && (e.wVirtualKeyCode==VK_F4)) {
			return false;
		}
	}
	return true; //keep running.
}