/*
This function handles the configfile translation and parsing into a configmap for later use

//
functions here
void fillMapWithDefaultValues()		fills the configmap with defaultvalues, so we dont have trouble if not everything is specified or some wrong parameters are used
int parseConfigFile(std::string configfilepath)		parses the configfile and puts the according values into the configmap
std::string configurationMapToString()		simple tostring method for the configmap
*/


#pragma once
//includes
#include <string>
#include <vector>
#include <fstream>
#include <iostream>

extern std::map<std::string, std::string> configurationMap;
extern void checkforloadFile(std::string filename, float vol);


//fills the configurationmap with the Default-values, if you want to add some or change them do it here
void fillMapWithDefaultValues() {
	//VOCODERAPPLICATION
	configurationMap["COMMANDINPUT"] = "CMD";
	configurationMap["ADRESS"] = "127.0.0.1";
	configurationMap["PORT"] = "7000";
	configurationMap["DELAYBETWEEENCOMMANDS"] = "500";
	configurationMap["MAXNUMOFBANDPASSBANDS"] = "16";
	configurationMap["ACTIVESTATE"] = "ACTIVE";
	configurationMap["PROCESSINGSTATE"] = "BYPASS";
	configurationMap["INPUTSOURCE"] = "FILEINPUT";
	configurationMap["CARRIER"] = "SINE";
	configurationMap["STARTASIO"] = "TRUE";


	//ASIO
	configurationMap["ASIODRIVER"] = "NONE";
	configurationMap["BUFFERSIZE"] = "64";
	configurationMap["INPUT"] = "MONO";
	configurationMap["OUTPUT"] = "MONO";
	configurationMap["INPUTCHANNEL"] = "1";
	configurationMap["OUTPUTCHANNEL"] = "12";
	configurationMap["PROCESSING"] = "MONO";
}

//Opens and Reads the Configfile
//returns -1 if file cannot be opened
//returns -2 if the file is empty after cleanup
//returns 0 else
int parseConfigFile(std::string configfilepath) {
	std::string PATH = configfilepath;
	std::vector<std::string> filecontent;
	std::ifstream fileIn;
	fileIn.open(PATH);
	if (!fileIn) {
		std::cout << PATH << " cannot be opened, does it exist?\n";
		return -1;
	}
	//read all the lines
	std::string s;
	while (!fileIn.eof()) {
		std::getline(fileIn,s);
		filecontent.push_back(s);
		s.clear();
	}
	fileIn.close();
	//clean the lines up
	std::vector<std::string> processedlines1;
	for (std::vector<std::string>::iterator it = filecontent.begin(); it != filecontent.end(); it++) {
		//Delete Whitespaces
		s.clear();
		s = *it;
		s.erase(std::remove_if(s.begin(), s.end(), ::isspace), s.end());
		//skip now empty lines
		if (s.size() < 1) {
			continue;
		}
		//skip linecomments
		if (s.at(0) == '#') {
			continue;
		}
		size_t pos = s.find("#");
		//delete the part that starts with #
		if (pos != std::string::npos) {
			s = s.substr(0, pos);
		}
		//check if the line has string=string format
		if (s.find('=') == std::string::npos) {
			continue;
		}
		//replace '$' with spaces
		for (int i = 0; i < s.length(); i++) {
			if (s.at(i) == '$') {
				s[i] = ' ';
			}
		}

		processedlines1.push_back(s);
	}//done cleanup
	//filecontent empty
	if (processedlines1.size() < 1) {
		return -2;
	}
	//fill the defaultvalues
	fillMapWithDefaultValues();
	//put the lines into the map
	for (std::vector<std::string>::iterator it = processedlines1.begin(); it != processedlines1.end(); it++) {
		s = *it;
		//find the =
		size_t pos = s.find('=');
		//check for audiofiles
		if (s.substr(0, pos).compare("FILENAME") == 0) {
			s = s.substr(pos + 1);
			size_t poscolon = s.find(">");
			float volume = 1.0;
			try {
				volume = std::stof(s.substr(poscolon + 1));
			}catch(std::exception e){}
			checkforloadFile(s.substr(0,poscolon), volume);
		}
		else {
			configurationMap[s.substr(0, pos)] = s.substr(pos + 1);
		}
	}
	return 0;
}

//to_string for ConfigMap
std::string configurationMapToString() {
	std::string s = "CONFIGURATION\n";
	for (std::map<std::string, std::string>::iterator it = configurationMap.begin(); it != configurationMap.end(); it++) {
		s.append("\t");
		s.append(it->first); s.append("->"); s.append(it->second); s.append("\n");
	}
	return s;
}