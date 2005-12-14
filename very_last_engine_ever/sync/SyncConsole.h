#pragma once
#include "sync.h"
#include "synctrack.h"
#include <windows.h>


#define WD 80
#define HT 25

class SyncEditor;

struct SyncConsole {
	void open();
	void cls();
	void flip();
	void put(short x, short y, WORD colour, string text); //put text
	void put(short x, short y, WORD colour, string text, char fill, DWORD length); //put text right-aligned
	void put(short x, short y, WORD colour, int number, bool signd, DWORD length); //put number right-aligned, zerofilled "(' '|'-')digit^length"
	void put(short x, short y, WORD colour, unsigned int number, DWORD length); //put number right-aligned, zerofilled "(' '|'-')digit^length"
	void fill(short x, short y, WORD colour, char text, DWORD length);
	void fillvertical(short x, short y, WORD colour, char text, DWORD length);
	void fillcolour(short x, short y, WORD colour, DWORD length);
	void offset(short x, short y) { xoff = x; yoff = y; }
	bool doevents(SyncEditor& editor);
	short xoff, yoff;
	HANDLE out;
	HANDLE in;
	INPUT_RECORD inbuf[128];
	CHAR_INFO buffer[80*50];
};

//little helper method for console colours
//specify colours as 0xRGB with 1 for colour and 0 for no colour. specify 2 for colour for extra mega turbo power intensity.
WORD colour(short fore, short back = 0);