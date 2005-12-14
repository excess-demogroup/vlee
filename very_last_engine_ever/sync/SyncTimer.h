#pragma once

//woohoo! an interface!
class SyncTimer {
public:
	virtual void pause() = 0;
	virtual void play() = 0;
	virtual float getposition() = 0;
	virtual void setposition(int pos) = 0;
	virtual bool isrunning() = 0;
};

#ifdef BASS_H

//assumes a fairly recent version of BASS!

class SyncTimerBASS_Stream : public SyncTimer {
public:
	SyncTimerBASS_Stream(HSTREAM h, float bpm, int rowsperbeat) : h(h), rps((bpm*rowsperbeat)/60.0f) {};
	void pause() {
		//BASS_Pause();
		BASS_ChannelPause(h);
	}
	void play() {
		//BASS_Start();
		BASS_ChannelPlay(h,false);
	}
	float getposition() {
		return BASS_ChannelBytes2Seconds(h,BASS_ChannelGetPosition(h)) * rps; 
	}
	void setposition(int pos) {
		BASS_ChannelSetPosition(h,BASS_ChannelSeconds2Bytes(h,pos/rps)+10); //+10 for silly not-totally-exact BASS bug.
	}
	bool isrunning() {
		return (BASS_ChannelIsActive(h)==BASS_ACTIVE_PLAYING);
	}
private:
	HSTREAM h;
	float rps;
};

#endif //BASS_H