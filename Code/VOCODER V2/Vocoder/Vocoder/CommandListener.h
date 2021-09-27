/*
class which listens either on commandline or on the specified address/port for osc-messages
everytime something is changed involving filters (Vocoderchannelamount, Carrier etc.) the audio gets muted for a short time
otherwise you will clearly hear the filters kicking in, which is quite unpleasant

functions here
void printAvailableCommands()	prints all commands onto the cmd
void singleWordCommand(std::string s)	for single word commands
void twoWordCommands(std::string s1, std::string s2)	for two word commands
Osclistenerclass:
	virtual void ProcessMessage(const osc::ReceivedMessage& m, const IpEndpointName& remoteEndpoint)	ignores the adress pattern and interprets the first to arguments
CommandListener()	Constructor, sets all needed things up,
void startListening()	starts listening on the specified source
*/




#pragma once
//includes
#include <map>
#include <string>
#include <iostream>
#include <thread>

//includes osc
#include "oscpack_1_1_0/ip/PacketListener.h"
#include "oscpack_1_1_0/osc/OscPrintReceivedElements.h"
#include "oscpack_1_1_0/osc/OscReceivedElements.h"
#include "oscpack_1_1_0/osc/OscPacketListener.h"
#include "oscpack_1_1_0/ip/UdpSocket.h"
#include <csignal>



//externals
extern std::map<std::string, std::string> configurationMap;
extern VocoderStatus* currentVocoderState;
extern ASIOInterface* ASI;
extern void setVolume(float vol);
extern void closeApplication();
extern void checkforloadFile(std::string filename, float vol);
extern void loadforplay(std::string s);
extern void playFile();


//INPUTS
#define CMD "CMD"
#define OSC "OSC"

//Commands
//Application
#define SHUTDOWN "Shutdown"
//State: Standby/Active
#define STATE "State"
#define STATEACTIVE "Active"
#define STATESTANDBY "Standby"
//Fileplayer
//Play: Start/Stop/Pause
#define PLAY "Play"
#define PLAYSTART "Start"
#define PLAYSTOP "Stop"
#define PLAYPAUSE "Pause"
//Load: Load into appl. or set up for playing
#define LOADINTOAPP "LoadIntoApp"
#define	LOADFORPLAY	"LoadForPlay"
//AudioOutput
#define VOLUME	"Volume"
#define VOLUMEMUTE	"Mute"
#define VOLUMEUNMUTE "Unmute"
//Vocoder
#define VOCODER	"Vocoder"
#define VOCODERON "On"
#define VOCODEROFF "Off"
//Input
#define INPUT "Input"
#define INPUTFILE "File"
#define INPUTMICROPHONE "Microphone"
//Carrier
#define CARRIER "Carrier"
#define CARRIERSINE "Sine"
#define CARRIERNOISE "Noise"
#define CARRIERSQUARE "Square"
#define CARRIERSAW "Saw"
//Postfiltering
#define POSTFILTERING "Postfiltering"
#define POSTFILTERINGON "On"
#define POSTFILTERINGOFF "Off"
//Channels
#define CHANNELAMOUNT "Channelamount"


//STATES
//activeState
#define ACTIVEstr "ACTIVE"
#define STANDBYstr "STANDBY"
//processingstate
#define BYPASSstr "BYPASS"
#define PROCESSINGstr "PROCESSING"
//inputsource
#define FILEINPUTstr "FILEINPUT"
#define MICROPHONEstr "MICROPHONE"
//carrier
#define SINEstr "SINE"
#define NOISEstr "NOISE"



//prints the available commands
void printAvailableCommands() {
	std::cout << "Available Commands:\n";
	std::cout << "Commands for the Application:\n";
	std::cout << "\t" << SHUTDOWN << "\tShuts down the application\n";
	std::cout << "\t" << STATE<<" "<<STATEACTIVE<<"|"<<STATESTANDBY << "\tSets the vocoder into standby- or activemode\n";
	std::cout<< "Commands for fileplaying:\n";
	std::cout << "\t" << LOADINTOAPP<<" <string filename>\tloads the given file, takes some time\n";
	std::cout << "\t" << LOADFORPLAY << " <string filename>\tprepares the given file to be played, if already playing this file will be played\n";
	std::cout << "\t" << PLAY<<" "<<PLAYSTART<<"|"<<PLAYSTOP<<"|"<<PLAYPAUSE << "\tplays/stops/pauses/unpauses the current file\n";
	std::cout << "Commands for Output:";
	std::cout << "\t" << VOLUME<<" <float volume>\tsets the outputvolume\n";
	std::cout << "Commands for the Vocoder\n";
	std::cout << "\t" << VOCODER <<" "<<VOCODERON<<"|"<<VOCODEROFF<< "\tturns the vocoder on(processing) or off(bypass)\n";
	std::cout << "\t" << INPUT <<" "<<INPUTFILE<<"|"<<INPUTMICROPHONE<< "\tsets the input for the vocoder\n";
	std::cout << "\t" << CARRIER <<" "<<CARRIERSINE<<"|"<<CARRIERNOISE<<"|"<<CARRIERSQUARE<<"|"<<CARRIERSAW<< "\tsets the carrier for the vocoder to sinewave, noise or square\n";
	std::cout << "\t" << POSTFILTERING <<" "<<POSTFILTERINGON<<"|"<<POSTFILTERINGOFF<<"\tturns postfiltering on or off\n";
	std::cout << "\t" << CHANNELAMOUNT <<" <int channels>\tsets the amount of channels for the vocoder\n";
}

std::string currentCommand;
bool listening = true;

//handles all commands which are only one word
void singleWordCommand(std::string s) {
	if (s.compare("help") == 0||s.compare("HELP")==0) {
		printAvailableCommands();
	}
	if (s.compare(SHUTDOWN) == 0) {
		ASI->stop();
		//closeApplication();
		listening = false;
		return;
	}
	//add additional commands here
	else { return; }
};

//handles all commands which are two words
void twoWordCommands(std::string s1, std::string s2) {
	//Standby or Active
	if (s1.compare(STATE) == 0) {
		if (s2.compare(STATEACTIVE) == 0) {
			//only do something if we are in standby
			if (currentVocoderState->activeState == STANDBY) {
				currentVocoderState->activeState = ACTIVE;
				//everything stays as it is except we stop fileplaying if we havent already
				ASI->resetFileProgress();
				currentVocoderState->fileplayerStatus = STOPPED;
				ASI->start(); //start asio
			}
			return;
		}
		else if (s2.compare(STATESTANDBY) == 0) {
			//only do something if we are active
			if (currentVocoderState->activeState == ACTIVE) {
				currentVocoderState->activeState = STANDBY;
				//everything stays as it is except we stop fileplaying if we havent already
				ASI->resetFileProgress();
				currentVocoderState->fileplayerStatus = STOPPED;
				ASI->stop(); //asio stop
			}
		}
		else { return; }
	}
	if (currentVocoderState->activeState == ACTIVE) {
		//Fileplayer: Play
		if (s1.compare(PLAY) == 0) {
			if (s2.compare(PLAYSTART) == 0) {
				//only do something if we arent already playing
				if (currentVocoderState->fileplayerStatus != PLAYING) {
					//ASI->makeFadeIn();
					currentVocoderState->fileplayerStatus = PLAYING;
					//if we are paused we will resume
					playFile();
				}
			}
			else if (s2.compare(PLAYSTOP) == 0) {
				//only do something if we havent already stopped
				if (currentVocoderState->fileplayerStatus != STOPPED) {
					//ASI->makeFadeOut();
					currentVocoderState->fileplayerStatus = STOPPED;
					ASI->resetFileProgress();
				}
			}
			else if (s2.compare(PLAYPAUSE) == 0) {
				//only do something if we havent stopped
				if (currentVocoderState->fileplayerStatus != STOPPED) {
					//if we are playing stop
					if (currentVocoderState->fileplayerStatus == PLAYING) {
						//ASI->makeFadeOut();
						currentVocoderState->fileplayerStatus = PAUSED;
					}
					//if we are paused resume playing
					else if (currentVocoderState->fileplayerStatus == PAUSED) {
						//ASI->makeFadeIn();
						currentVocoderState->fileplayerStatus = PLAYING;
					}
				}
			}
			return;
		}
		//Fileplayer: LoadIntoApp
		else if (s1.compare(LOADINTOAPP) == 0) {
			//Load with no volume adjustment
			checkforloadFile(s2, 1.0);
			return;
		}
		//Fileplayer: LoadForPlay
		else if (s1.compare(LOADFORPLAY) == 0) {
			//reset fileprogress
			ASI->resetFileProgress();
			loadforplay(s2);
			return;
		}
		//AudioOutput
		else if (s1.compare(VOLUME) == 0) {
			if (s2.compare(VOLUMEMUTE) == 0) {
				//ASI->makeFadeOut();
				ASI->mute();
			}
			else if(s2.compare(VOLUMEUNMUTE) == 0) {
				//ASI->makeFadeIn();
				ASI->mute();
				return;
			}
			//no checking if volume is in reasonable bounds, do at your own risk
			try {
				float vol = std::stof(s2);
				setVolume(vol);
			}
			catch (std::exception e) {}
		return;
		}
		//Vocoder
		else if (s1.compare(VOCODER) == 0) {
			//if we arent already vocoding switch to vocoding
			if (s2.compare(VOCODERON) == 0&&currentVocoderState->processingState!=VOCODING) {
				ASI->mute();
				currentVocoderState->processingState = VOCODING;
				Sleep(0.5); //to avoid clicks
				ASI->mute();
			}
			//if we arent already in bypass
			else if (s2.compare(VOCODEROFF) == 0 && currentVocoderState->processingState!=BYPASS) {
				ASI->mute();
				currentVocoderState->processingState = BYPASS;
				Sleep(500);
				ASI->mute();
			}
			return;
		}
		//Input
		else if (s1.compare(INPUT) == 0) {
			//if we arent already playing from file, switch
			if (s2.compare(INPUTFILE) == 0 && currentVocoderState->inputSource!=FILEINPUT) {
				ASI->mute();
				currentVocoderState->inputSource = FILEINPUT;
				Sleep(500);
				ASI->mute();
			}
			else if (s2.compare(INPUTMICROPHONE) == 0 && currentVocoderState->inputSource!=MICROPHONE) {
				ASI->mute();
				currentVocoderState->inputSource = MICROPHONE;
				Sleep(500);
				ASI->mute();
			}
			return;
		}
		//carrier
		else if (s1.compare(CARRIER) == 0) {
			if (s2.compare(CARRIERSINE) == 0 && currentVocoderState->carrier != SINE) {
				ASI->mute();
				currentVocoderState->carrier = SINE;
				Sleep(500);
				ASI->mute();
			}
			else if (s2.compare(CARRIERNOISE) == 0 && currentVocoderState->carrier != NOISE) {
				ASI->mute();
				currentVocoderState->carrier = NOISE;
				Sleep(500);
				ASI->mute();
			}
			else if (s2.compare(CARRIERSQUARE) == 0 && currentVocoderState->carrier != SQUARE) {
				ASI->mute();
				currentVocoderState->carrier = SQUARE;
				Sleep(500);
				ASI->mute();
			}
			else if (s2.compare(CARRIERSAW) == 0 && currentVocoderState->carrier != SAW) {
				ASI->mute();
				currentVocoderState->carrier = SAW;
				Sleep(500);
				ASI->mute();
			}
			return;
		}
		//Postfiltering
		else if (s1.compare(POSTFILTERING) == 0) {
			if (s2.compare(POSTFILTERINGON) == 0 && currentVocoderState->postProcessing!=POSTFILTER) {
				//ASI->makeFadeOut();
				ASI->setpostfilter(true);
				//currentVocoderState->postProcessing = POSTFILTER;
				//ASI->makeFadeIn();
			}
			else if (s2.compare(POSTFILTERINGOFF) == 0 && currentVocoderState->postProcessing!= NOPOSTFILTER) {
				//ASI->makeFadeOut();
				ASI->setpostfilter(false);
				//currentVocoderState->postProcessing = NOPOSTFILTER;
				//ASI->makeFadeIn();
			}
			return;
		}
		//channelamount
		else if (s1.compare(CHANNELAMOUNT) == 0) {
			try {
				int ch = std::stoi(s2);
				ASI->setVocoderChannel(ch);
			}
			catch (std::exception e) { return; }
		}
		return;
	}
	return;

}

//Class for OSC
class OscListener : public osc::OscPacketListener {
protected:
	//this function just unpacks the osc message and calls the according function
	virtual void ProcessMessage(const osc::ReceivedMessage& m, const IpEndpointName& remoteEndpoint) {
		(void)remoteEndpoint;
		//ignore the adress pattern
		//only need to consider 2 args, the first is a string the second can be a string or float or int
		osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
		std::string s1 = "";
		if (arg->IsString()) {
			try {
				s1 = arg->AsString();
				if (s1.compare("Shutdown") == 0) {
					std::raise(SIGINT);
				}
			}
			catch (std::exception e) {
				//if the first isnt a string just return
				return;
			}
			arg++;
			if (arg == m.ArgumentsEnd()) {
				singleWordCommand(s1);
			}
			else if (arg->IsFloat()) {
				try {
					twoWordCommands(s1, std::to_string(arg->AsFloat()));
				}
				catch (std::exception e) {}
			}
			else if (arg->IsString()) {
				try {
					twoWordCommands(s1, arg->AsString());
				}
				catch (std::exception e) {}
			}
			else if (arg->IsInt32()) {
				try {
					twoWordCommands(s1, std::to_string(arg->AsInt32()));
				}
				catch (std::exception e) {}
			}
			else {
				return;
			}
		}
	}
};

//sets up everything for listening on the given source
//Constructor: gets the parameter from the configurationmap
//startlistening(): start listening on the chosen source for commands
class CommandListener {
private:
	int listeningSource = 0; //1: CMD; 2: OSC;
	int timeToSleep = 100;//delay after the next command will be processed

public:
	~CommandListener() {}
	CommandListener() {
		std::string input = configurationMap.at("COMMANDINPUT");
		if (input.compare(CMD) == 0) {
			listeningSource = 0;
		}
		else if (input.compare(OSC)==0) {
			listeningSource = 1;
		}
		//IF OTHER SOURCES NEEDED INSERT HERE
		else {}
		
		//Set the delaytime betweencommands
		timeToSleep = std::stoi(configurationMap.at("DELAYBETWEENCOMMANDS"));


		//set the status with the values from the ConfigurationMap
		//activeState
		input = configurationMap.at("ACTIVESTATE");
		if (input.compare(ACTIVEstr) == 0) {
			currentVocoderState->activeState = ACTIVE;
		}
		else if (input.compare(STANDBYstr) == 0) {
			currentVocoderState->activeState = STANDBY;
		}
		//add additional activestates here

		//default
		else {
			currentVocoderState->activeState = ACTIVE;
		}

		//processingState
		input = configurationMap.at("PROCESSINGSTATE");
		if (input.compare(BYPASSstr) == 0) {
			currentVocoderState->processingState = BYPASS;
		}
		else if (input.compare(PROCESSINGstr) == 0) {
			currentVocoderState->processingState = VOCODING;
		}
		//add additional processingstates here

		//default
		else {
			currentVocoderState->processingState = BYPASS;
		}

		//inputsource
		input = configurationMap.at("INPUTSOURCE");
		if (input.compare(FILEINPUTstr) == 0) {
			currentVocoderState->inputSource = FILEINPUT;
		}
		else if (input.compare(MICROPHONEstr) == 0) {
			currentVocoderState->inputSource = MICROPHONE;
		}
		//add additional inputsources here

		//default
		else {
			currentVocoderState->inputSource = MICROPHONE;
		}

		//carrier
		input = configurationMap.at("CARRIER");
		if (input.compare(SINEstr) == 0) {
			currentVocoderState->carrier = SINE;
		}
		else if (input.compare(NOISEstr) == 0) {
			currentVocoderState->carrier = NOISE;
		}
		//add additional carriers here

		//default
		else {
			currentVocoderState->carrier = SINE;
		}

		if (listeningSource == 1) {

		}
	};
	//start listening on the chosen input cmd/osc
	void startListening() {
		if (listeningSource == 0) {
			//printAvailableCommands();
			std::cout << "expecting commands on console.\n\ttype 'HELP'/'help' for available commands\n";
			while (listening) {
				std::getline(std::cin, currentCommand);
				//chech if we have a space
				size_t pos = currentCommand.find(" ");
				if (pos == std::string::npos) {
					singleWordCommand(currentCommand);
				}
				else {
					std::string s1 = currentCommand.substr(0, pos);
					std::string s2 = currentCommand.substr(pos + 1);
					//check if we have only one space and dont do anything if we have more
					//dont do this, filename could contain spaces
					//if (s2.find(" ") == std::string::npos) 
					twoWordCommands(s1, s2);
				}
				Sleep(timeToSleep);
			}

		}
		else if (listeningSource == 1) {
			std::string adr = configurationMap.at("ADRESS");
			int port = std::stoi(configurationMap.at("PORT"));
			OscListener listener;
			UdpListeningReceiveSocket s(IpEndpointName(adr.c_str(), port), &listener);
			std::cout << "expecting commands on: "<<adr<<":"<<port<<"\n";
			s.RunUntilSigInt();
		}
	
	}
	
};