/*
Here are all functions directly working on ASIO

//functions here:
bool loadAsioDriverManually() loads the driver via commandline
long init_asio_static_data(DriverInfo* asioDriverInfo)	sets up all data for the ASIO Driver
bool createASIOBuffers(DriverInfo* asioDriverInfo)		creates the buffers for ASIO
ASIOTime* bufferSwitchTimeInfo(ASIOTime* timeInfo, long index, ASIOBool processNow)		callback method which reads the inputbuffer delegates it to the vocoding function and writes it back to the outputbuffer
void bufferSwitch(long index, ASIOBool processNow)	callback methods
void sampleRateChanged(ASIOSampleRate sRate)		resets samplerate if changed
long asioMessages(long selector, long value, void* message, double* opt) callback method
void ASIOMute()	mutes/unmutes the application processing still happens
void ASIOSetVolume() sets the volume
void ASIOResetFileProgress() resets the fileprogress
void VocoderSetChannel(int i) sets the amount of channels used starting with 1 turns postfiltering always off
void setPostFiltering(bool filteron) turns postfiltering on or off


*/
#pragma once

//includes
#include "asiosys.h"
#include "asio.h"
#include "asiodrivers.h"
#include "ProcessingFunctions.h"

//include own stuff
#include "VocoderStatus.h"

ASIOSampleType sampletype;




bool needfadeOut = false;
bool needfadeIn = false;

//THE MAXIMUM OF SUPPORTED IN/OUTPUTS, IF YOU WANT TO CHANGE THEM DO IT HERE
const int MAXINPUTCHANNELS = 2;
const int MAXOUTPUTCHANNELS = 2;


//Struct for Driver
typedef struct DriverInfo {
	//ASIOInit()
	ASIODriverInfo	driverInfo = {0};
	//ASIOGetChannels()
	long			inputChannels;
	long			outputChannels;
	//ASIOGetBufferSize()
	long			minSize;
	long			maxSize;
	long			preferredSize;
	long			granularity;
	//ASIOGetSampleRate()
	ASIOSampleRate	sampleRate;
	//ASIOOutputReady()
	bool			postOutput;
	//ASIOCreateBuffers()
	long			inputBuffers; //the number of actual inputbuffers
	long			outputBuffers; //the number of actual outputbuffers
	ASIOBufferInfo	bufferInfos[MAXINPUTCHANNELS + MAXOUTPUTCHANNELS];
	//ASIOGetChannelInfo()
	ASIOChannelInfo channelInfos[MAXINPUTCHANNELS + MAXOUTPUTCHANNELS];
	//ASIOGetLatencies()
	long			inputLatency;
	long			outputLatency;
	bool			stopped=false;

	int				NumberOfProcessedSamples = 0;

	float			volumePost=1.0;

}DriverInfo;

//external stuff
extern AsioDrivers* asioDrivers;
extern ASIOCallbacks asioCallbacks;
extern DriverInfo asioDriverInfo;
extern std::vector<float> SignalBufferIn;
extern std::vector<float> SignalBufferOut;
extern std::vector<float>* currentAudioFileVector;
extern int* currentAudioFileSize;
extern VocoderStatus* currentVocoderState;
extern float OUTPUTVOLUME;
bool loadAsioDriver(char* name);




//displays the 5 first available ASIODrivers and lets the user select one
//if you wish to show more, change it here
//returns true if the Driver could be loaded
//returns false else
bool loadAsioDriverManually() {
	//Grab the available Drivernames
	int NumDriversToLoad = 5;
	char* name1 = new char[32]; name1[0] = 0;
	char* name2 = new char[32]; name2[0] = 0;
	char* name3 = new char[32]; name3[0] = 0;
	char* name4 = new char[32]; name4[0] = 0;
	char* name5 = new char[32]; name5[0] = 0;
	char* name6 = new char[32]; name6[0] = 0;
	char* name7 = new char[32]; name7[0] = 0;
	char* name8 = new char[32]; name8[0] = 0;
	char* name9 = new char[32]; name9[0] = 0;
	char* name10 = new char[32]; name10[0] = 0;
	char* drivernames[] = { name1,name2,name3,name4,name5,name6,name7,name8,name9,name10 };
	bool driverloaded = false;
	while (driverloaded == false) {
		asioDrivers->getDriverNames(drivernames, NumDriversToLoad);
		int maxNumAvailableDrivers = 0;
		for (int i = 0; i < NumDriversToLoad; i++) {
			if (strlen(drivernames[i]) != 0) {
				std::cout << "(" << i << ") " << drivernames[i] << "\n";
				maxNumAvailableDrivers++;
			}

		}
		if (maxNumAvailableDrivers == 0) {
			std::cout << "No ASIO-driver available...\n";
			delete[] name1, name2, name3, name4, name5, name6, name7, name8, name9, name10;
			//delete drivernames;
			return false;
		}
		std::cout << "Please select the driver you want to use or exit the application\n";
		int driverNumToBeLoaded = -1;
		while (driverNumToBeLoaded < 0) {
			std::cin >> driverNumToBeLoaded;
			std::cout << "Loading " << drivernames[driverNumToBeLoaded] << "...\n";
		}
		if (asioDrivers->loadDriver(drivernames[driverNumToBeLoaded]) == true) {
			driverloaded = true;
		}
		else {
			std::cout << "Could not load driver!\n";
		}
	}
	delete[] name1, name2, name3, name4, name5, name6, name7, name8, name9, name10;
	//delete drivernames;
	return true;
}

//loads all the static driver data
long init_asio_static_data(DriverInfo* asioDriverInfo) {
	//collect the informational data of the driver
	//get the number of available channels
	if(ASIOGetChannels(&asioDriverInfo->inputChannels, &asioDriverInfo->outputChannels)==ASE_OK){
		//get the usable buffersizes
		if (ASIOGetBufferSize(&asioDriverInfo->minSize, &asioDriverInfo->maxSize, &asioDriverInfo->preferredSize, &asioDriverInfo->granularity) == ASE_OK) {
			//get the currently selected sample rate
			if (ASIOGetSampleRate(&asioDriverInfo->sampleRate) == ASE_OK) {
				if (asioDriverInfo->sampleRate <= 0.0 || asioDriverInfo->sampleRate > 96000.0) {
					//Driver does not store its internal sample rate, so set it to a known one
					//Usually you should check beforehand, that the selected sample rate is valid
					//with ASIOCanSample()
					if (ASIOSetSampleRate(44100.0) == ASE_OK) {
						if (ASIOGetSampleRate(&asioDriverInfo->sampleRate) == ASE_OK) {

						}
						else - 6;
					}
					else - 5;
				}
				//check wether the driver requires the ASIOOutputReady() optimization
				//can be used by the driver to reduce output latency by one block
				if (ASIOOutputReady() == ASE_OK) {
					asioDriverInfo->postOutput = true;
				}
				else {
					asioDriverInfo->postOutput = false;
				}
				return 0;

			}
			return -3;
		}
		return -2;
	}
	return -1;
}

//creates all the necessary Buffers
bool createASIOBuffers(DriverInfo* asioDriverInfo) {
	//set Buffersize
	asioDriverInfo->preferredSize = (long)stoi(configurationMap.at("BUFFERSIZE"));
	ASIOBufferInfo* info = asioDriverInfo->bufferInfos;
	std::string ins, outs;
	std::string inputs = configurationMap.at("INPUTCHANNEL");
	std::string outputs = configurationMap.at("OUTPUTCHANNEL");
	//only use valid channels
	for (int i = 0; i < inputs.size(); i++) {
		char c = inputs.at(i);
		int ch = std::atoi(&c);
		if (ch <= MAXINPUTCHANNELS && ch >= 0) {
			ins.push_back(c);
		}
	}
	for (int i = 0; i < outputs.size(); i++) {
		char c = outputs.at(i);
		int ch = std::atoi(&c);
		if (ch <= MAXOUTPUTCHANNELS && ch >= 0) {
			outs.push_back(c);
		}
	}
	//if we dont have outputchannels this doesnt make sense
	if (outs.size() < 1) {
		return false;
	}
	//assuming we dont have duplicates!!!
	asioDriverInfo->inputBuffers = ins.size();
	asioDriverInfo->outputBuffers = outs.size();
	//prepare Buffers
	for (int i = 0; i < asioDriverInfo->inputBuffers; i++, info++) {
		char c = ins.at(i);
		info->isInput = ASIOTrue;
		info->channelNum = std::atoi(&c) -1;
		info->buffers[0] = info->buffers[1] = 0;
	}
	for (int i = 0; i < asioDriverInfo->outputBuffers; i++, info++) {
		char c = outs.at(i);
		info->isInput = ASIOFalse;
		info->channelNum = std::atoi(&c) -1;
		info->buffers[0] = info->buffers[1] = 0;
	}
	//create the actual buffers
	ASIOError res = ASIOCreateBuffers(asioDriverInfo->bufferInfos, asioDriverInfo->inputBuffers + asioDriverInfo->outputBuffers, asioDriverInfo->preferredSize, &asioCallbacks);
	if (res == ASE_OK) {
		//write the bufferinfos to channels
		for (int i = 0; i < asioDriverInfo->inputBuffers + asioDriverInfo->outputBuffers; i++) {
			asioDriverInfo->channelInfos[i].channel = asioDriverInfo->bufferInfos[i].channelNum;
			asioDriverInfo->channelInfos[i].isInput = asioDriverInfo->bufferInfos[i].isInput;
			res = ASIOGetChannelInfo(&asioDriverInfo->channelInfos[i]);
			if (res != ASE_OK) {
				break;
			}
		}
		//get the latencies
		if (res == ASE_OK) {
			res = ASIOGetLatencies(&asioDriverInfo->inputLatency, &asioDriverInfo->outputLatency);
		}
	}
	if (res == ASE_OK) {
		//Buffers are created, we can setUp the Stuff for Vocoding
		setupForVocoding(asioDriverInfo->preferredSize);
		return true;
	}
	return false;
}


// conversion from 64 bit ASIOSample/ASIOTimeStamp to double float
#if NATIVE_INT64
#define ASIO64toDouble(a)  (a)
#else
const double twoRaisedTo32 = 4294967296.;
#define ASIO64toDouble(a)  ((a).lo + (a).hi * twoRaisedTo32)
#endif
//CALLBACKS------------------------------------------------
ASIOTime* bufferSwitchTimeInfo(ASIOTime* timeInfo, long index, ASIOBool processNow)
{	
	
	int16_t* buf16 = NULL;
	int32_t* buf32 = NULL;
	float* buf32f = NULL;
	double* buf64 = NULL;
	
	
	//This should not be called if the Vocoder is in Standby so no need to check that
	// buffer size in samples
	long buffSize = asioDriverInfo.preferredSize;
	//We only enter this part if the Vocoder is accessing the Microphone
	if (currentVocoderState->inputSource==MICROPHONE) {
		//get the Inputs into the SignalBuffer
		for (int i = 0; i < asioDriverInfo.inputBuffers + asioDriverInfo.outputBuffers; i++) {
			if (asioDriverInfo.channelInfos[i].isInput) {
				switch (asioDriverInfo.channelInfos[i].type) {
				case ASIOSTInt16LSB:
					
					for (int j = 0; j < buffSize; j++) {
						//get the value, cast it to 16bit Integer and divide it by the maxValue
						SignalBufferIn[j] = (float)*((int16_t*)asioDriverInfo.bufferInfos[i].buffers[index] + j)/(int16_t)0x7fff; //value should now be [-1.0,1.0]
					}
					break;
				case ASIOSTInt24LSB:		// used for 20 bits as well
					for (int j = 0; j < buffSize; j++) {
						//get the value, cast it to 32bit Integer and divide it by the maxValue
						SignalBufferIn[j] = (float)*((int32_t*)asioDriverInfo.bufferInfos[i].buffers[index] + j) / (int32_t)0x7fffffff; //value should now be [-1.0,1.0]
					}
					break;
				case ASIOSTInt32LSB:
					for (int j = 0; j < buffSize; j++) {
						//get the value, cast it to 32bit Integer and divide it by the maxValue
						SignalBufferIn[j] = (float)*((int32_t*)asioDriverInfo.bufferInfos[i].buffers[index] + j)/(int32_t)0x7fffffff; //value should now be [-1.0,1.0]
					}
					break;
				case ASIOSTFloat32LSB:		// IEEE 754 32 bit float, as found on Intel x86 architecture
					for (int j = 0; j < buffSize; j++) {
						//get the value, cast it to float and divide it by the maxValue
						SignalBufferIn[j] = (float)*((float*)asioDriverInfo.bufferInfos[i].buffers[index] + j)/(float)FLT_MAX; //value should now be [-1.0,1.0]
					}
					break;
				case ASIOSTFloat64LSB: 		// IEEE 754 64 bit double float, as found on Intel x86 architecture
					for (int j = 0; j < buffSize; j++) {
						//get the value, cast it double and divide it by the maxValue
						SignalBufferIn[j] = (float)*((double*)asioDriverInfo.bufferInfos[i].buffers[index] + j)/(double)DBL_MAX; //Value should now be [-1.0,1.0]
					}
					break;

					// these are used for 32 bit data buffer, with different alignment of the data inside
					// 32 bit PCI bus systems can more easily used with these
				case ASIOSTInt32LSB16:		// 32 bit data with 18 bit alignment
				case ASIOSTInt32LSB18:		// 32 bit data with 18 bit alignment
				case ASIOSTInt32LSB20:		// 32 bit data with 20 bit alignment
				case ASIOSTInt32LSB24:		// 32 bit data with 24 bit alignment
					//TODO
					break;

				case ASIOSTInt16MSB:
					//TODO
					break;
				case ASIOSTInt24MSB:		// used for 20 bits as well
					//TODO
					break;
				case ASIOSTInt32MSB:
					//TODO
					break;
				case ASIOSTFloat32MSB:		// IEEE 754 32 bit float, as found on Intel x86 architecture
					//TODO
					break;
				case ASIOSTFloat64MSB: 		// IEEE 754 64 bit double float, as found on Intel x86 architecture
					//TODO
					break;

					// these are used for 32 bit data buffer, with different alignment of the data inside
					// 32 bit PCI bus systems can more easily used with these
				case ASIOSTInt32MSB16:		// 32 bit data with 18 bit alignment
				case ASIOSTInt32MSB18:		// 32 bit data with 18 bit alignment
				case ASIOSTInt32MSB20:		// 32 bit data with 20 bit alignment
				case ASIOSTInt32MSB24:		// 32 bit data with 24 bit alignment
					//TODO
					break;
				}
			}
		}
	}
	else if (currentVocoderState->inputSource==FILEINPUT) {
		if (currentVocoderState->fileplayerStatus == PLAYING) { //FILECURRENTLY PLAYING A FILE
			if (currentAudioFileVector != nullptr) {
				for (int i = 0; i < buffSize; i++) {
					SignalBufferIn[i] = (*currentAudioFileVector)[(asioDriverInfo.NumberOfProcessedSamples + i) % *currentAudioFileSize]; //this will loop the audio
				}
			}
		}
		else { //CURRENTLY STOPPED OR PAUSED
			for (int i = 0; i < buffSize; i++) {
				SignalBufferIn[i] =0;
			}
		}
	}
	else {
		//IF ADDITIONAL INPUTSOURCE IS NEEDED ADD IT HERE
	}

	if (currentVocoderState->processingState==VOCODING) {
		vocode();
	}

	else if(currentVocoderState->processingState==BYPASS){//if we are in Bypass Mode we dont really do much
		for (int i = 0; i < buffSize; i++) {
			SignalBufferOut[i] = SignalBufferIn[i];
		}
	}
	else {
		//IF ADDITIONAL PROCESSINGSTATE IS NEEDED ADD IT HERE
	}

	//do fade out first, no matter if we need to fade in, fade a whole buffer in or out, not very elegant i know, but it will do the job
	if(needfadeOut){
		for (int i = 0; i < buffSize; i++) {
			SignalBufferOut[i] = SignalBufferOut[i] * asioDriverInfo.volumePost * (-i / buffSize + 1);
		}
		needfadeOut = false;
	}
	//fade in only if we dont need to fade out
	else if (needfadeOut&&!needfadeIn) {
		for (int i = 0; i < buffSize; i++) {
			SignalBufferOut[i] = SignalBufferOut[i] * asioDriverInfo.volumePost * (i / buffSize);
		}
		needfadeIn = false;
	}
	//no fade ins or outs
	else{
		for (int i = 0; i < buffSize; i++) {
			SignalBufferOut[i] = SignalBufferOut[i] * asioDriverInfo.volumePost;
		}
	}


	// the actual processing callback.
	// Beware that this is normally in a seperate thread, hence be sure that you take care
	// about thread synchronization. This is omitted here for simplicity.
	/*static long processedSamples = 0;

	// store the timeInfo for later use
	asioDriverInfo.tInfo = *timeInfo;

	// get the time stamp of the buffer, not necessary if no
	// synchronization to other media is required
	if (timeInfo->timeInfo.flags & kSystemTimeValid)
		asioDriverInfo.nanoSeconds = ASIO64toDouble(timeInfo->timeInfo.systemTime);
	else
		asioDriverInfo.nanoSeconds = 0;

	if (timeInfo->timeInfo.flags & kSamplePositionValid)
		asioDriverInfo.samples = ASIO64toDouble(timeInfo->timeInfo.samplePosition);
	else
		asioDriverInfo.samples = 0;
	
	if (timeInfo->timeCode.flags & kTcValid)
		asioDriverInfo.tcSamples = ASIO64toDouble(timeInfo->timeCode.timeCodeSamples);
	else
		asioDriverInfo.tcSamples = 0;

	// get the system reference time
	asioDriverInfo.sysRefTime = get_sys_reference_time();*/
	/*
#if WINDOWS && _DEBUG
	// a few debug messages for the Windows device driver developer
	// tells you the time when driver got its interrupt and the delay until the app receives
	// the event notification.
	static double last_samples = 0;
	char tmp[128];
	sprintf(tmp, "diff: %d / %d ms / %d ms / %d samples                 \n", asioDriverInfo.sysRefTime - (long)(asioDriverInfo.nanoSeconds / 1000000.0), asioDriverInfo.sysRefTime, (long)(asioDriverInfo.nanoSeconds / 1000000.0), (long)(asioDriverInfo.samples - last_samples));
	OutputDebugString(tmp);
	last_samples = asioDriverInfo.samples;
#endif
	*/

	// output the processed audio
	for (int i = 0; i < asioDriverInfo.inputBuffers + asioDriverInfo.outputBuffers; i++)
	{
		
		if (asioDriverInfo.bufferInfos[i].isInput == false)
		{
			// OK do processing for the outputs only
			switch (asioDriverInfo.channelInfos[i].type)
			{
			case ASIOSTInt16LSB:
				//zero the buffer
				memset(asioDriverInfo.bufferInfos[i].buffers[index], 0, (size_t)buffSize * 2);
				buf16 = (int16_t*)asioDriverInfo.bufferInfos[i].buffers[index];
				for (int j = 0; j < buffSize; j++) {
					//get the value, multiply it with the maxValue of Int16 and cast it to int16
					buf16[j] = (int16_t)0x7fff*SignalBufferOut[j];
				}
				break;
			case ASIOSTInt24LSB:		// used for 20 bits as well
				//zero the buffer
				memset(asioDriverInfo.bufferInfos[i].buffers[index], 0, (size_t)buffSize * 3);
				//TODO
				buf32 = (int32_t*)asioDriverInfo.bufferInfos[i].buffers[index];
				for (int j = 0; j < buffSize; j++) {
					buf32[j] = (int32_t)0x7fffffff * SignalBufferOut[j];
				}
				break;
			case ASIOSTInt32LSB:
				//zero the buffer
				memset(asioDriverInfo.bufferInfos[i].buffers[index], 0, (size_t)buffSize * 4);
				buf32 = (int32_t*)asioDriverInfo.bufferInfos[i].buffers[index];
				for (int j = 0; j < buffSize; j++) {
					//get the value, multiply it with the maxvalue of Int32 and cast it to int32
					buf32[j] = (int32_t)0x7fffffff * SignalBufferOut[j];
				}
				
				break;
			case ASIOSTFloat32LSB:		// IEEE 754 32 bit float, as found on Intel x86 architecture
				//zero the buffer
				memset(asioDriverInfo.bufferInfos[i].buffers[index], 0, (size_t)buffSize * 4);
				buf32f = (float*)asioDriverInfo.bufferInfos[i].buffers[index];
				for (int j = 0; j < buffSize; j++) {
					//get the value, multiply it with the maxValue of float and cast it to float
					buf32f[j] = (float)(FLT_MAX*SignalBufferOut[j]);
				}
				break;
			case ASIOSTFloat64LSB: 		// IEEE 754 64 bit double float, as found on Intel x86 architecture
				//zero the buffer
				memset(asioDriverInfo.bufferInfos[i].buffers[index], 0, (size_t)buffSize * 8);
				buf64 = (double*)asioDriverInfo.bufferInfos[i].buffers[index];
				for (int j = 0; j < buffSize; j++) {
					//get the value, multiply it with the maxValue of double and cast it to double
					buf64[j] = (double)(DBL_MAX*SignalBufferOut[j]);
				}
				break;

				// these are used for 32 bit data buffer, with different alignment of the data inside
				// 32 bit PCI bus systems can more easily used with these
			case ASIOSTInt32LSB16:		// 32 bit data with 18 bit alignment
			case ASIOSTInt32LSB18:		// 32 bit data with 18 bit alignment
			case ASIOSTInt32LSB20:		// 32 bit data with 20 bit alignment
			case ASIOSTInt32LSB24:		// 32 bit data with 24 bit alignment
				//memset(asioDriverInfo.bufferInfos[i].buffers[index], 0, buffSize * 4);
				break;

			case ASIOSTInt16MSB:
				//memset(asioDriverInfo.bufferInfos[i].buffers[index], 0, buffSize * 2);
				break;
			case ASIOSTInt24MSB:		// used for 20 bits as well
				//memset(asioDriverInfo.bufferInfos[i].buffers[index], 0, buffSize * 3);
				break;
			case ASIOSTInt32MSB:
				//memset(asioDriverInfo.bufferInfos[i].buffers[index], 0, buffSize * 4);
				break;
			case ASIOSTFloat32MSB:		// IEEE 754 32 bit float, as found on Intel x86 architecture
				//memset(asioDriverInfo.bufferInfos[i].buffers[index], 0, buffSize * 4);
				break;
			case ASIOSTFloat64MSB: 		// IEEE 754 64 bit double float, as found on Intel x86 architecture
				//memset(asioDriverInfo.bufferInfos[i].buffers[index], 0, buffSize * 8);
				break;

				// these are used for 32 bit data buffer, with different alignment of the data inside
				// 32 bit PCI bus systems can more easily used with these
			case ASIOSTInt32MSB16:		// 32 bit data with 18 bit alignment
			case ASIOSTInt32MSB18:		// 32 bit data with 18 bit alignment
			case ASIOSTInt32MSB20:		// 32 bit data with 20 bit alignment
			case ASIOSTInt32MSB24:		// 32 bit data with 24 bit alignment
				//memset(asioDriverInfo.bufferInfos[i].buffers[index], 0, buffSize * 4);
				break;
			}
		}
	}

	// finally if the driver supports the ASIOOutputReady() optimization, do it here, all data are in place
	if (asioDriverInfo.postOutput)
		ASIOOutputReady();
	/*
	if (processedSamples >= asioDriverInfo.sampleRate * TEST_RUN_TIME)	// roughly measured
		asioDriverInfo.stopped = true;
	else
		processedSamples += buffSize;
	*/
	if (currentVocoderState->fileplayerStatus == PLAYING) { //ONLY ADVANCE IF WE ARE PLAYING A FILE
		asioDriverInfo.NumberOfProcessedSamples += buffSize;
	}
	return 0L;
}
void bufferSwitch(long index, ASIOBool processNow)
{	// the actual processing callback.
	// Beware that this is normally in a seperate thread, hence be sure that you take care
	// about thread synchronization. This is omitted here for simplicity.

	// as this is a "back door" into the bufferSwitchTimeInfo a timeInfo needs to be created
	// though it will only set the timeInfo.samplePosition and timeInfo.systemTime fields and the according flags
	ASIOTime  timeInfo;
	memset(&timeInfo, 0, sizeof(timeInfo));

	// get the time stamp of the buffer, not necessary if no
	// synchronization to other media is required
	if (ASIOGetSamplePosition(&timeInfo.timeInfo.samplePosition, &timeInfo.timeInfo.systemTime) == ASE_OK)
		timeInfo.timeInfo.flags = kSystemTimeValid | kSamplePositionValid;

	bufferSwitchTimeInfo(&timeInfo, index, processNow);
}
void sampleRateChanged(ASIOSampleRate sRate)
{
	// do whatever you need to do if the sample rate changed
	// usually this only happens during external sync.
	// Audio processing is not stopped by the driver, actual sample rate
	// might not have even changed, maybe only the sample rate status of an
	// AES/EBU or S/PDIF digital input at the audio device.
	// You might have to update time/sample related conversion routines, etc.
	//reset the samplerate
	ASIOSetSampleRate(44100);
}
long asioMessages(long selector, long value, void* message, double* opt)
{
	// currently the parameters "value", "message" and "opt" are not used.
	long ret = 0;
	switch (selector)
	{
	case kAsioSelectorSupported:
		if (value == kAsioResetRequest
			|| value == kAsioEngineVersion
			|| value == kAsioResyncRequest
			|| value == kAsioLatenciesChanged
			// the following three were added for ASIO 2.0, you don't necessarily have to support them
			|| value == kAsioSupportsTimeInfo
			|| value == kAsioSupportsTimeCode
			|| value == kAsioSupportsInputMonitor)
			ret = 1L;
		break;
	case kAsioResetRequest:
		// defer the task and perform the reset of the driver during the next "safe" situation
		// You cannot reset the driver right now, as this code is called from the driver.
		// Reset the driver is done by completely destruct is. I.e. ASIOStop(), ASIODisposeBuffers(), Destruction
		// Afterwards you initialize the driver again.
		//asioDriverInfo.stopped;
		//asioDriverInfo->stopped;  // In this sample the processing will just stop
		ret = 1L;
		break;
	case kAsioResyncRequest:
		// This informs the application, that the driver encountered some non fatal data loss.
		// It is used for synchronization purposes of different media.
		// Added mainly to work around the Win16Mutex problems in Windows 95/98 with the
		// Windows Multimedia system, which could loose data because the Mutex was hold too long
		// by another thread.
		// However a driver can issue it in other situations, too.
		ret = 1L;
		break;
	case kAsioLatenciesChanged:
		// This will inform the host application that the drivers were latencies changed.
		// Beware, it this does not mean that the buffer sizes have changed!
		// You might need to update internal delay data.
		ret = 1L;
		break;
	case kAsioEngineVersion:
		// return the supported ASIO version of the host application
		// If a host applications does not implement this selector, ASIO 1.0 is assumed
		// by the driver
		ret = 2L;
		break;
	case kAsioSupportsTimeInfo:
		// informs the driver wether the asioCallbacks.bufferSwitchTimeInfo() callback
		// is supported.
		// For compatibility with ASIO 1.0 drivers the host application should always support
		// the "old" bufferSwitch method, too.
		ret = 1;
		break;
	case kAsioSupportsTimeCode:
		// informs the driver wether application is interested in time code info.
		// If an application does not need to know about time code, the driver has less work
		// to do.
		ret = 0;
		break;
	}
	return ret;
}

//tells the processingblock to fade in or out
void fadeIn() {	needfadeIn = true; };
void fadeOut() { needfadeOut = true; };
//Mutes the output or unmutes it, depending on the currentoutputvolume set in MAIN
void ASIOMute() {
	if (asioDriverInfo.volumePost == 0.0) {
		asioDriverInfo.volumePost = OUTPUTVOLUME;
	}
	else {
		asioDriverInfo.volumePost = 0.0;
	}
}
//tells to switch the volume
void ASIOSetVolume() {
	asioDriverInfo.volumePost = OUTPUTVOLUME;
}
//resets the played file to the beginning, it just restarts - no stop
void ASIOResetFileProgress() {
	asioDriverInfo.NumberOfProcessedSamples = 0;
}
//set the channelamount for the vocoder
void VocoderSetChannel(int i) {
	
	
	if (currentVocoderState->processingState == BYPASS) {
		setChannel(i);
	}
	else {
		
		if (currentVocoderState->postProcessing == POSTFILTER) {
			//turn of the postfilter otherwise the nasty sounds last even longer
			currentVocoderState->postProcessing = NOPOSTFILTER;
		}
		//if just switch during vocoding we get some not so cool sounds so we need to delay a bit to flush them out of the system
		ASIOMute();
		currentVocoderState->fileplayerStatus = PAUSED;
		setChannel(i);
		Sleep(750); //wait a bit for nasty sounds to pass
		ASIOMute();
		currentVocoderState->fileplayerStatus = PLAYING;
		
	}
}
void setPostFiltering(bool filteron) {
	//turning on the filter while playing can produce loud clicks to avoid that we mute the application
	if (filteron) {
		ASIOMute();
		currentVocoderState->postProcessing = POSTFILTER;
		Sleep(500);
		ASIOMute();
	}
	else { //turning it off is no problem
		currentVocoderState->postProcessing = NOPOSTFILTER;
	}
}