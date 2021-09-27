/*
This holds just some variables for the loaded files so we dont clutter up other files too much, will be accessed by the loading functions in MAIN.cpp
*/

#pragma once

//includes
#include <map>
#include <string>
#include <vector>

std::map<std::string, std::vector<float>> audioFiles;
std::map<std::string, int> audioFilesSize;
std::vector<float>* currentAudioFileVector;
int* currentAudioFileSize;