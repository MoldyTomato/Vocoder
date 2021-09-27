/*
Class to keep track of the overall state of the vocoder
includes the overall state of the application, although this isnt used, the processingstate, carrier and states of the fileplayer and if we use postfiltering

*/

#pragma once
//includes
#include <string>


//activeState
#define ACTIVE		0
#define STANDBY		1
//use 0 - 9 for activeState
//processingState
#define BYPASS		10
#define VOCODING	11
//use 10-19 for processingState
//inputSource
#define FILEINPUT	20
#define MICROPHONE	21
//use 20-29 for inputSource
//carrier
#define SINE		30
#define NOISE		31
#define SAW			32
#define SQUARE		33
//use 30-39 for the carrier
//status as a fileplayer
#define STOPPED		40
#define PLAYING		41
#define	PAUSED		42
//use 40-49 for the status as fileplayer
//status for PostProcessing
#define POSTFILTER 50
#define NOPOSTFILTER 51
//use 50-59 for postprocessing


class VocoderStatus {
private:
public:
	int activeState = ACTIVE;
	int processingState = BYPASS;
	int inputSource = MICROPHONE;
	int carrier = SINE;
	int fileplayerStatus = STOPPED;
	int postProcessing = POSTFILTER;
	int NumOfFilterBands = 0;
	//if additional States are needed insert them here and modify the according functions
	VocoderStatus() {};
	~VocoderStatus() {};
};
