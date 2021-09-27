/*
This is the main entry point for the application!
if want to specify the configfilepath and driver on execution
do it like this: Application.exe -f configfile -d driver

//
functions here:
void setVolume(float vol)		set the outputvolume of the whole application by calling the according function in the ASIOInterface.h
void loadfile(std::string filename, float vol)		loads a given file if it is supported with the specified volume multiplier
void checkforloadFile(std::string filename, float vol)		checks if the file is already loaded and if it isnt it calls loadfile(std::string filename, float vol)
void loadforplay(std::string s)		sets up the pointer for the given file and prepares it for playing	!if we are already playing a file this will start immediately
void playFile()		tells the application to play the chosen file	!if we are already playing this file this will reset and play from the beginning
*/




//MAKE SURE YOU SET _CRT_SECURE_NO_WARNINGS AS A PREPROCESSOR-DIRECTIVE
//MAKE SURE YOU SET THE CHARACTERSET TO MULTI-BYTE INSTEAD OF UNICODE

//for Memoryleak detection
//#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#ifdef _DEBUG
#define DEBUG_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
#define new DEBUG_NEW
#endif



//includes
#include <vector>
#include <string>
#include <map>
#include "AudioFile/AudioFile.h" //for Reading WAV and AIFF


//includes own stuff
#include "ConfigFileParser.h"
#include "ASIOInterface.h"
#include "CommandListener.h"
#include "VocoderStatus.h"
#include "FilesForPlaying.h"

//externals
extern std::map<std::string, std::vector<float>> audioFiles;
extern std::map<std::string, int> audioFilesSize;
extern std::vector<float>* currentAudioFileVector;
extern int* currentAudioFileSize;


//Variables
std::string CONFIGFILEPATH = "Config.config";
std::string DRIVER = "NONE";
AudioFile<float> audioFile;
CommandListener* commandlistener;


//Variables for external use
ASIOInterface* ASI;
VocoderStatus* currentVocoderState;
std::map<std::string, std::string> configurationMap;
DriverInfo asioDriverInfo;
ASIOCallbacks asioCallbacks;
std::vector<float> SignalBufferIn; //Buffer for the input of one ASIOBuffer
std::vector<float> SignalBufferOut;//Buffer for the output of one ASIOBuffer

float OUTPUTVOLUME = 1.0;

//sets the Outputvolume and tells the ASIOInterface to change it
void setVolume(float vol) {
	OUTPUTVOLUME = vol;
	ASI->setVolume();
}
//loads the audiofile and writes to cmd if the samplerate is unsupported
void loadfile(std::string filename, float vol) {
	audioFile.load(filename);
	if (audioFile.getSampleRate() != (uint32_t)44100) {
		std::cout << "Unsupported Samplerate for: " << filename << "\n";
		audioFiles.erase(audioFiles.find(filename));//if you want to do resampling do it here
		return;
	}
	bool mono = audioFile.isMono();
	std::vector<float> data;
	int totalSamples = audioFile.getNumSamplesPerChannel();
	data.resize(totalSamples);
	audioFilesSize[filename] = totalSamples;
	for (int i = 0; i < totalSamples; i++) {
		if (mono) {
			data[i] = audioFile.samples[0][i]*vol;
		}
		else {
			data[i] = (float)vol*((double)audioFile.samples[0][i] + (double)audioFile.samples[1][i]) / 2.0; //if stereo just average them
		}
	}
	if (data.size() > 0) {
		audioFiles[filename] = data;
	}
	else {
		audioFiles.erase(audioFiles.find(filename));
	}
	//std::cout << filename << " loaded\n";


}
//checks if there is already a file with this name and loads it
void checkforloadFile(std::string filename, float vol) {
	//check if we already have a file named like this
	if (audioFiles.find(filename) == audioFiles.end()) {
		//insert name and some dummy vector
		std::vector<float> z = {};
		audioFiles[filename] = z;
		//now start thread for loading
		//std::thread t(loadfile, filename, vol);
		//t.detach();
		loadfile(filename, vol);
	}
	return;
}
//checks if there is such a file and sets the according Variables, if we are already playing it switches immediatly
void loadforplay(std::string s) {
	//check if we already have a file like this
	if (audioFiles.find(s) != audioFiles.end()) {
		currentAudioFileVector = &audioFiles.at(s);
		currentAudioFileSize = &audioFilesSize.at(s);
	}
}
//Checks if we have chosen a file and starts playing it
void playFile() {
	//are we already playing
	//if (currentVocoderState->fileplayerStatus == PLAYING) {
	//	return;
	//}
	//check if we have nullptr
	if (currentAudioFileVector != nullptr && currentAudioFileSize != nullptr) {
		currentVocoderState->fileplayerStatus = PLAYING;
		ASI->resetFileProgress(); //start from the beginning
	}
	return;
}

//MAIN ENTRY POINT FOR THE APPLICATION
//can be called with params -f <configfilepath> -d <ASIOdriver>
int main(int argc, char* argv[]) {
	//do we have arguments
	if (argc > 1) {
		std::vector<std::string> args;
		for (int i = 1; i < argc; i++) {
			args.push_back(argv[i]);
		}
		//parse the arguments
		//they should be like:
		//-f configfile -d driver or vice versa
		for (std::vector<std::string>::iterator it = args.begin(); it != args.end(); it++) {
			std::string s = *it;
			//file
			if (s.compare("-f") == 0) {
				it++;
				CONFIGFILEPATH = *it;
			}
			//driver
			if (s.compare("-d") == 0) {
				it++;
				DRIVER = *it;
			}
		}
	}//done with cmdarguments
	//Parse the config-file
	if (parseConfigFile(CONFIGFILEPATH) == -1) { //skip the -2 error
		return -1;
	}
	//std::cout<<configurationMapToString();

	//Set the correct size for the processing Buffers
	SignalBufferIn.resize(stoi(configurationMap.at("BUFFERSIZE")));
	SignalBufferOut.resize(stoi(configurationMap.at("BUFFERSIZE")));

	currentVocoderState = new VocoderStatus();

	//Set VocoderStates
	//Active?
	if (configurationMap.at("ACTIVESTATE").compare("ACTIVE")==0) {
		currentVocoderState->activeState = ACTIVE;
	}
	else {
		currentVocoderState->activeState = STANDBY;
	}
	//Processing
	if (configurationMap.at("PROCESSINGSTATE").compare("VOCODING")==0) {
		currentVocoderState->processingState = VOCODING;
	}
	else {
		currentVocoderState->processingState = BYPASS;
	}
	//input
	if (configurationMap.at("INPUTSOURCE").compare("FILEINPUT")==0) {
		currentVocoderState->inputSource = FILEINPUT;
	}
	else {
		currentVocoderState->inputSource = MICROPHONE;
	}
	//carrier
	if (configurationMap.at("CARRIER").compare("SINE")==0) {
		currentVocoderState->carrier = SINE;
	}
	else {
		currentVocoderState->carrier = NOISE;
	}
	//postprocessing
	currentVocoderState->postProcessing = NOPOSTFILTER;






	//AsioInterface
	bool success=false;
	ASI = new ASIOInterface(&success);
	if (success == false) {
		std::cout << "failed\n";
		return -1;
	}
	
	//set sampleRate
	if (ASI->setSampleRate() != true) {
		std::cout << "Could not set Samplerate\n";
		return -1;
	}
	//setupCallbacks
	ASI->setCallbacks();
	//create Buffers
	if (ASI->createBuffers() == false) {
		std::cout << "Buffercreation failed\n";
		return -1;
	}
	//ASI->infoOut();
	//std::cout << "success\n";
	if (currentVocoderState->activeState == ACTIVE) {
		ASI->start();
		std::cout << "asio started";
	}


	commandlistener = new CommandListener();
	std::cout << "ready\n";
	commandlistener->startListening(); //this blocks there cant be anything after here!!

	delete commandlistener;
	delete ASI;
	delete currentVocoderState;

	return 0;
}