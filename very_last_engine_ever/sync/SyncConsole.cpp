#include "syncconsole.h"
#include "synceditor.h"
#define _CRT_SECURE_NO_DEPRECATE
#include <windows.h>
#include "uglyswitchhack.h"


struct Coord : public _COORD { Coord(short x, short y) { X = x; Y = y; }; };

//little helper method for console colours
//specify colours as 0xRGB with 1 for colour and 0 for no colour. specify 2 for colour for extra mega turbo power intensity.
WORD colour(short fore, short back) {
	WORD attr = 0;
	if(fore&0x300) attr += FOREGROUND_RED;
	if(fore&0x030) attr += FOREGROUND_GREEN;
	if(fore&0x003) attr += FOREGROUND_BLUE;
	if(fore&0x222) attr += FOREGROUND_INTENSITY;
	if(back&0x300) attr += BACKGROUND_RED;
	if(back&0x030) attr += BACKGROUND_GREEN;
	if(back&0x003) attr += BACKGROUND_BLUE;
	if(back&0x222) attr += BACKGROUND_INTENSITY;
	return attr;
}


void SyncConsole::open() {
	AllocConsole();
	out = GetStdHandle(STD_OUTPUT_HANDLE);
	in = GetStdHandle(STD_INPUT_HANDLE);
	SetConsoleTitle("SYNCTRACKER 2.0 MEGA DELUXE");
	SetConsoleScreenBufferSize(out, Coord(WD,HT));
	CONSOLE_SCREEN_BUFFER_INFO gak;
	GetConsoleScreenBufferInfo(out,&gak);
	SMALL_RECT win = {0, 0, gak.dwMaximumWindowSize.X, gak.dwMaximumWindowSize.Y};
	SetConsoleWindowInfo(out, true, &win);
    SetConsoleMode(in, ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT);
	CONSOLE_CURSOR_INFO cursor = {10, false};
	SetConsoleCursorInfo(out,&cursor);
	COORD zork = GetLargestConsoleWindowSize(out);
	SMALL_RECT moo = {400,400,400+zork.X,400+zork.Y};
	SetConsoleWindowInfo(out,TRUE,&moo);

}

void SyncConsole::put(short x, short y, WORD colour, string text) {
	int idx = x+xoff+(y+yoff)*WD;
	for(unsigned int i=0; i<text.length(); i++, idx++) {
		buffer[idx].Char.AsciiChar = text[i];
		buffer[idx].Attributes = colour;
	}
	//WriteConsoleOutputCharacter(out,text.c_str(),(DWORD)text.length(),Coord(x+xoff,y+yoff),&dummy);
	//FillConsoleOutputAttribute(out,colour,(DWORD)text.length(),Coord(x+xoff,y+yoff),&dummy);
}

void SyncConsole::fill(short x, short y, WORD colour, char text, DWORD length) {
	int idx = x+xoff+(y+yoff)*WD;
	for(unsigned int i=0; i<length; i++, idx++) {
		buffer[idx].Char.AsciiChar = text;
		buffer[idx].Attributes = colour;
	}
	//FillConsoleOutputCharacter(out,text,length,Coord(x+xoff,y+yoff),&dummy);
	//FillConsoleOutputAttribute(out,colour,length,Coord(x+xoff,y+yoff),&dummy);
}

void SyncConsole::fillvertical(short x, short y, WORD colour, char text, DWORD length) {
	int idx = x+xoff+(y+yoff)*WD;
	for(unsigned int i=0; i<length; i++, idx+=WD) {
		buffer[idx].Char.AsciiChar = text;
		buffer[idx].Attributes = colour;
	}
}

void SyncConsole::fillcolour(short x, short y, WORD colour, DWORD length) {
	int idx = x+xoff+(y+yoff)*WD;
	for(unsigned int i=0; i<length; i++, idx++) {
		buffer[idx].Attributes = colour;
	}	
}

void SyncConsole::put(short x, short y, WORD colour, string text, char fill, DWORD length) {
	string moo;
	if(length>text.length()) moo.resize(length-text.length(),fill);
	moo.reserve(length+60);
	moo.append(text);
	put(x,y,colour,moo);
}

//puts zero padded numbers of length + (signd => 1)
void SyncConsole::put(short x, short y, WORD colour, int number, bool signd, DWORD length) {
	char def[10]; 
	wsprintf(def, "%c%%0%ux", ((number<0)?'-':' '), length); //contains sign symbol
	char moo[10];
	wsprintf(moo,def,abs(number)); //abs is necessary - wsprintf doesn't do negative hex but we do.
	put(x,y,colour,moo+(signd?0:1));
}

//puts zero padded UNSIGNED numbers of length
void SyncConsole::put(short x, short y, WORD colour, unsigned int number, DWORD length) {
	char def[10]; 
	wsprintf(def, "%%0%ux", length);
	char moo[10];
	wsprintf(moo,def,number);
	put(x,y,colour,moo);
}


void SyncConsole::flip() {
	SMALL_RECT r = {0, 0, WD-1, HT-1};
	WriteConsoleOutput(out,buffer,Coord(WD,HT),Coord(0,0),&r);
}

void SyncConsole::cls() {
	ZeroMemory(&buffer,sizeof(CHAR_INFO)*WD*HT);
	//FillConsoleOutputCharacter(out, ' ', WD*HT, Coord(0,0), &dummy);
	//FillConsoleOutputAttribute(out, 0, WD*HT, Coord(0,0), &dummy);
}

bool SyncConsole::doevents(SyncEditor& editor) {

	DWORD i, numread, numavail;
	bool stop = false;

	while(GetNumberOfConsoleInputEvents(in,&numavail), ((numavail) && (!stop))) { //hooray for the comma operator!
		ReadConsoleInput(in, inbuf, 128, &numread);
		//put(5,6,colour(0x002),"moo");
		for (i = 0; (i < numread) && (!stop); i++) 
		{
			switch(inbuf[i].EventType) { 
				ifcase(KEY_EVENT) { // keyboard input 
					stop = !editor.keyevent(inbuf[i].Event.KeyEvent); 
				} elsecase(MOUSE_EVENT) { // mouse input 
					stop = !editor.mouseevent(inbuf[i].Event.MouseEvent); 
				} elsecase(WINDOW_BUFFER_SIZE_EVENT) { // scrn buf. resizing 
					stop = !editor.resizeevent(inbuf[i].Event.WindowBufferSizeEvent); 
				}

				//} elsecase(FOCUS_EVENT) {  // disregard focus events 
				//} elsecase(MENU_EVENT) { // disregard menu events 
			} 
		}
	}
	return !stop;
}