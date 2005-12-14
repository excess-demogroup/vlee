#pragma once
#include "sync.h"
#include "synctrack.h"
#include <windows.h>
#include "syncconsole.h"

class SyncEditor :
	public Sync
{
public:
	virtual void showEditor();
	bool doEvents();
	void moveTo(int order);
	SyncEditor(string filename, SyncTimer& timer);
	~SyncEditor(void);
	virtual void update();
	SyncTrack& getTrack(string name, string group, int digits, bool signd) {
		if(!editoropen) return Sync::getTrack(name,group,digits,signd);
		MessageBox(NULL,"Hey, don't you getTrack when the editor is already open!!","omg wtf rolf cum bbq",MB_OK);
		return tracks.begin()->second; //return something stupid.
	}
protected:
	bool mouseevent(MOUSE_EVENT_RECORD e); 
	bool resizeevent(WINDOW_BUFFER_SIZE_RECORD e); 
	bool keyevent(KEY_EVENT_RECORD e);
	void unedit();
	void saveData();
	void applyorderchange();
	void paint();
	void forceposition();

	vector<int> patternoffsets;
	SyncConsole con;
	bool orderfocus;
	bool editing;
	bool editoropen;
	int curpos;
	int curptn;
	int curtrack;
	int selstarttrack;
	int selstartrow;
	int selendtrack;
	int selendrow;
	bool selecting;
	bool selection;
	bool visible;
	trackMap::iterator curtrack_iter;

	__int64 entry;
	//unsigned int entrypoint;
friend struct SyncConsole;
};
