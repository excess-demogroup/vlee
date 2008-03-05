#pragma once

#ifdef BASS_H

class BassTimer : public sync::Timer
{
public:
	BassTimer(HSTREAM stream, float bpm, int rowsPerBeat) : stream(stream)
	{
		rowRate = (bpm / 60.0f) * rowsPerBeat;
	}
	
	// BASS hooks
	void  pause()           { BASS_ChannelPause(stream); }
	void  play()            { BASS_ChannelPlay(stream, false); }
	float getTime()         { return BASS_ChannelBytes2Seconds(stream, BASS_ChannelGetPosition(stream)); }
	float getRow()          { return getTime() * rowRate; }
	void  setRow(float row) { BASS_ChannelSetPosition(stream, BASS_ChannelSeconds2Bytes(stream, row / rowRate)); }
	bool  isPlaying()       { return (BASS_ChannelIsActive(stream) == BASS_ACTIVE_PLAYING); }
private:
	HSTREAM stream;
	float rowRate;
};
#endif //BASS_H