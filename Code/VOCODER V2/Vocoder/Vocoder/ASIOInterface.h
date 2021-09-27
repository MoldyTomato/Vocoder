/*
Interface class for the ASIO-related things

//functions here
ASIOInterface(bool* success)		Constructor: tries to load the driver specified in the configfile and initializes all necessary data
	if not successfull calls the function to manually load a driver
bool loadManually()		call the according function in ASIOFunctions to load a driver manuually via the command line
bool setSampleRate()	sets the ASIO samplerate to 44100
void setCallbacks()		sets up the ASIO callbacks
bool createBuffers()	creates the ASIObuffers
void start()		starts the ASIO Process
void stop()		stops the ASIO Process
void mute()		mutes/unmutes the whole output
void setVolume()	sets the volume of output
void resetFileProgress() resets the progress of the played file
void setVocoderChannel(int i) sets the amount of channels used starting with 1
void makeFadeIn()	makes fade in //NOT IMPLEMENTED
void makeFadeOut()	makes fade out //NOT IMPLEMENTED
void setpostfilter(bool filteron) turns on postfiltering
~ASIOInterface()	destroys the interface and cleans everything up
void infoOut()		prints ASIO DriverInfo
const char* ASIOSampleTypeToString(ASIOSampleType sampletype)		converts sampletype to char*
*/


#pragma once

//includes
#include <stdlib.h>

//includes my stuff
#include "ASIOFunctions.h"

extern std::map<std::string, std::string> configurationMap;
extern DriverInfo asioDriverInfo;
extern ASIOCallbacks asioCallbacks;

class ASIOInterface {
private:
	bool asioStarted = false;
	bool loadManually() {
		return loadAsioDriverManually();
	}


public:
	//constructor: loads the driver and sets up internal asio-data
	//returns the result in the parameter
	ASIOInterface(bool* success) {
		std::string s = configurationMap["ASIODRIVER"];
		int i = s.length();
		char* c = new char[(size_t)i + 1];
		strcpy(c, s.c_str());
		if (!loadAsioDriver(c)) {
			std::cout << "Driver could not be loaded\n";
			if (this->loadManually() == false) {
				*success = false;
				delete[] c;
				return;
			}
		}
		delete[] c;
		//init
		if (ASIOInit(&asioDriverInfo.driverInfo) != ASE_OK) {
			std::cout << "Could not initialize ASIO-Device: " << asioDriverInfo.driverInfo.errorMessage << "\n";
			*success = false;
			return;
		}
		//initData
		if (init_asio_static_data(&asioDriverInfo) != 0) {
			std::cout << "Could not initialize ASIO-Data\n";
			*success = false;
			return;
		}
		*success = true;
	}
	//Sets the Samplerate to 44100
	//returns true if it could be set
	//false otherwise
	bool setSampleRate() {
		double sampleRate = 44100.0;
		if (ASIOCanSampleRate(sampleRate) == ASE_OK) {
			if (ASIOSetSampleRate(sampleRate) == ASE_OK) {
				if (ASIOGetSampleRate(&asioDriverInfo.sampleRate) == ASE_OK) {
					return true;
				}
			}
		}
		return false;
	}
	
	//Sets up the callbacks
	void setCallbacks() {
		asioCallbacks.bufferSwitch = bufferSwitch;
		asioCallbacks.bufferSwitchTimeInfo = bufferSwitchTimeInfo;
		asioCallbacks.sampleRateDidChange = sampleRateChanged;
		asioCallbacks.asioMessage = asioMessages;
	}
	//creates Buffers
	bool createBuffers() {
		return createASIOBuffers(&asioDriverInfo);
	}
	//starts the ASIOProcessing
	void start() {
		if (asioStarted==false) {
			if (ASIOStart() != ASE_OK) {
				std::cout << "ASIO could not be started\n";
				exit(EXIT_FAILURE);
			}
			asioStarted = true;
		}
	}
	
	//stops the ASIOProcessing
	void stop() {
		if (asioStarted == true) {
			if (ASIOStop() != ASE_OK) {
				std::cout << "ASIO could not be stopped\n";
				exit(EXIT_FAILURE);
			}
			asioStarted = false;
		}
	}

	//mutes the audio, but it is still processed
	void mute() {
		ASIOMute();
	}
	//set volume
	void setVolume() {
		ASIOSetVolume();
	}
	//resets the currently playing file
	void resetFileProgress() {
		ASIOResetFileProgress();
	}
	//sets the amount of bandpassses
	void setVocoderChannel(int i) {
		VocoderSetChannel(i);
	}
	//tells to fade in
	void makeFadeIn() {
		fadeIn();
	};
	//tells to fade out
	void makeFadeOut() {
		fadeOut();
	};
	void setpostfilter(bool filteron) {
		setPostFiltering(filteron);
	}

	//destroys the interface
	~ASIOInterface() {
		ASIODisposeBuffers();
		ASIOExit;
		asioDrivers->removeCurrentDriver();
		delete asioDrivers;
	}

	//toStringmethof for asioDriverInfo
	void infoOut(){
		std::cout << "ASIODRIVERINFO:\n";
		std::cout << "ASIOVersion: "<<asioDriverInfo.driverInfo.asioVersion<<"\n";
		std::cout << "DriverVersion: " << asioDriverInfo.driverInfo.driverVersion << "\n";
		std::cout << "Errormessage: " << asioDriverInfo.driverInfo.errorMessage << "\n";
		std::cout << "Name: " << asioDriverInfo.driverInfo.name << "\n";
		std::cout << "SysRef: " << asioDriverInfo.driverInfo.sysRef << "\n";
		std::cout << "Channels:\n";
		std::cout << "Input: available: " << asioDriverInfo.inputChannels << " used: " <<asioDriverInfo.inputBuffers<<"\n";
		std::cout << "Output: available: " << asioDriverInfo.outputChannels << " used: " << asioDriverInfo.outputBuffers << "\n";
		std::cout << "Buffersize:\n";
		std::cout << "MinSize: " << asioDriverInfo.minSize << ", maxSize: " << asioDriverInfo.maxSize << ", prefferedSize: " << asioDriverInfo.preferredSize << "\n";
		std::cout << "SampleRate: " << asioDriverInfo.sampleRate << "\n";
		std::cout << "Latencies:\n";
		std::cout << "Input: " << asioDriverInfo.inputLatency << ", Output: " << asioDriverInfo.outputLatency << "\n";
		for (int i = 0; i < asioDriverInfo.inputBuffers + asioDriverInfo.outputBuffers; i++) {
			std::cout << "Buffer " << i<<" is "; //the index of the Buffer
			std::cout << (asioDriverInfo.channelInfos[i].isInput ? "Input" : "Output")<<"buffer for "; //wether the channel is Input or output
			std::cout << "Channel " << (asioDriverInfo.channelInfos[i].channel + 1); //The In/Output-Channel as numbered on the device (starting with 1...)
			std::cout << " with the ASIOSampleType: " << ASIOSampleTypeToString(asioDriverInfo.channelInfos[i].type) << "\n";
		}
	}
	//toStringMethod for the different Sampletypes
	const char* ASIOSampleTypeToString(ASIOSampleType sampletype) {
		switch (sampletype) {
		case ASIOSTInt16MSB:	return "ASIOSTInt16MSB";
		case ASIOSTInt24MSB:	return "ASIOSTInt24MSB";
		case ASIOSTInt32MSB:	return "ASIOSTInt32MSB";
		case ASIOSTFloat32MSB:	return "ASIOSTFloat32MSB";
		case ASIOSTFloat64MSB:	return "ASIOSTFloat64MSB";
		case ASIOSTInt32MSB16:	return "ASIOSTInt32MSB16";
		case ASIOSTInt32MSB18:	return  "ASIOSTInt32MSB18";
		case ASIOSTInt32MSB20:	return "ASIOSTInt32MSB20";
		case ASIOSTInt32MSB24:	return "ASIOSTInt32MSB24";
		case ASIOSTInt16LSB:	return "ASIOSTInt16LSB";
		case ASIOSTInt24LSB:	return "ASIOSTInt24LSB";
		case ASIOSTInt32LSB:	return "ASIOSTInt32LSB";
		case ASIOSTFloat32LSB:	return "ASIOSTFloat32LSB";
		case ASIOSTFloat64LSB:	return "ASIOSTFloat64LSB";
		case ASIOSTInt32LSB16:	return "ASIOSTInt32LSB16";
		case ASIOSTInt32LSB18:	return "ASIOSTInt32LSB18";
		case ASIOSTInt32LSB20:	return "ASIOSTInt32LSB20";
		case ASIOSTInt32LSB24:	return "ASIOSTInt32LSB24";
		case ASIOSTDSDInt8LSB1: return "ASIOSTDSDInt8LSB1";
		case ASIOSTDSDInt8MSB1: return "ASIOSTDSDInt8MSB1";
		case ASIOSTDSDInt8NER8: return "ASIOSTDSDInt8NER8";
		default: return "";
		}
	}
};