/*
This handles all the reading of filter related things, should be called from the setupvocoding function in ProcessingFunctions.h

//functions here
void readFilterFromFile(int * maxfilterAmount, std::vector<std::vector<int>> * coeffCount_Num, std::vector<std::vector<int>> * coeffCount_Denom, std::vector<std::vector<std::vector<double>>> * allCoeffs_Num, std::vector<std::vector<std::vector<double>>>* allCoeffs_Denom, int maxAmountCoeffs, std::vector<std::vector<double>> * denom0Coeff)
	this reads the filters from the file into the according data structures
void ReadLowPassForEnvelope(int  numCoeffsLoPass, int  numCoeffsLo256bufs, int * numOfNum, int * numOfDenom, std::vector<double> * NumCoeffs, std::vector<double> *DenomCoeffs, double* Denom0)
	this reads the lowpassfilter from its file and puts everything into the according data structures
void ReadCenterFrequencies(int NumOfFilters, std::vector<std::vector<float>>* freqs)
	this reads the centerfrequencies from the filterbank files
void ReadFilterWeights(int FilterAmount, std::vector<std::vector<float>>* filterweights)
	reads the filterweights from its file and puts them into the given datastructure
*/


#pragma once

//includes
#include <string>
#include <iostream>
#include <fstream>
#include <filesystem>

//externals
extern std::map<std::string, std::string> configurationMap;

//some stuff
std::string rootfolder = "Filters/";
std::vector<std::string> filepaths;
std::vector<std::string> filecontent;

//params:
//maxfilterAmount: pointer to the variable for the maxfilteramount
//coeffCount_Num: pointer to 2D-Vector for NumeratorCountPerFilter
//coeffCount_Denom: pointer to 2D-Vector for DenominatorCountPerFilter
//allCoeffs_Num: pointer to 3D-Vector for all NumeratorCoefficients
//allCoeffs_Denom: pointer to 3D-Vector for all DenominatorCoefficients
//maxCoeffs: maxCoefficientCount
//denom0Coeff: pointer to 2DVector for the first Denominator
void readFilterFromFile(int * maxfilterAmount, std::vector<std::vector<int>> * coeffCount_Num, std::vector<std::vector<int>> * coeffCount_Denom, std::vector<std::vector<std::vector<double>>> * allCoeffs_Num, std::vector<std::vector<std::vector<double>>>* allCoeffs_Denom, int maxAmountCoeffs, std::vector<std::vector<double>> * denom0Coeff) {
	try {
		rootfolder = configurationMap.at("FILTERLOCATION") + "/";
	}
	catch (std::exception e) {}
	//how many filters are there
	int filterAmount = -1;
	std::vector<std::string> filesInFolder;
	
	for (const auto& entry : std::filesystem::directory_iterator(rootfolder)) {
			filesInFolder.push_back(entry.path().string().substr(rootfolder.size()));
	}//all files from folder collected
	
	for (std::vector<std::string>::iterator it = filesInFolder.begin(); it != filesInFolder.end(); it++) {
		std::string s = *it;
		std::string f = "FilterBank";
		if (s.substr(0, f.size()).compare(f) == 0) { //filename starts with "FilterBank"
			size_t pos = s.find("_"); //find the underscore
			try {
				int x = std::stoi(s.substr(pos+1));
				//std::cout << "FILTERBANKS: " << x;
				if (x > filterAmount) {
					filterAmount = x;
				}
			}catch(std::exception e){}
		}
	}//now we have the Amount of Filters
	if (filterAmount < 0) { //if we dont have filters starting the vocoder doesnt make sense
		std::cout << "Could not find filterfiles, do they exist?\n";
		exit(EXIT_FAILURE);
	}
	//get the amount from config
	if (configurationMap.at("MAXNUMOFBANDPASSBANDS").compare("AUTO") != 0) {
		int filterAmountFromConfig = std::stoi(configurationMap.at("MAXNUMOFBANDPASSBANDS"));
		if (filterAmountFromConfig > filterAmount || filterAmountFromConfig < 1) {
			std::cout << "specified amount of filters in the configfile is not available, existing will be used\n";
		}
		else {
			filterAmount = filterAmountFromConfig;
		}
	}
	//now we have the correct maximal filteramount
	*maxfilterAmount = filterAmount; //write back
	
	//resize the coefficientcounters
	(*coeffCount_Num).resize(filterAmount);
	(*coeffCount_Denom).resize(filterAmount);
	for (int i = 0; i < filterAmount; i++) {
		(*coeffCount_Num)[i].resize(filterAmount);
		(*coeffCount_Denom)[i].resize(filterAmount);
		for (int j = 0; j < filterAmount; j++) {
			//zero them just in case
			(*coeffCount_Num)[i][j] = 0;
			(*coeffCount_Denom)[i][j]=0;
		}
	}
	
	//resize the 2DVector for the first Denominator
	(*denom0Coeff).resize(filterAmount);
	for (int i = 0; i < filterAmount; i++) {
		(*denom0Coeff)[i].resize(filterAmount);
		for (int j = 0; j < filterAmount; j++) {
			//write 1 instead of zero
			(*denom0Coeff)[i][j] = 1;
		}
	}

	//create the filenames according to naming scheme
	filepaths.resize(filterAmount);
	for (int i = 0; i < filterAmount; i++) {
		filepaths[i] = rootfolder + "FilterBank_" + std::to_string(i + 1)+".txt";
	}//filepaths created
	
	//open each file
	for (int i = 0; i < filterAmount; i++) {
		std::ifstream file;
		file.open(filepaths[i]);
		if (!file) { //file cant be opened
			std::cout << "could not open: " << filepaths[i] << "\n";
			//we already wrote zeros at init so skip this
			file.close();
			continue;
		}
		//we opened the file successfully
		filecontent.clear();
		while (!file.eof()) {
			std::string s;
			std::getline(file, s);
			filecontent.push_back(s);
		}
		file.close();
		//file is read
		//check for the numerator or Denominator count
		for (std::vector<std::string>::iterator it = filecontent.begin(); it != filecontent.end(); it++) {
			int filterofFilterBank = 0;
			std::string s = *it;
			//erase whitespaces
			s.erase(std::remove_if(s.begin(), s.end(), ::isspace), s.end());
			//check if we get something like "Filterband:"
			if (s.find("Filterband:") != std::string::npos) {
				//find the colon
				size_t pos = s.find(":");
				int filterNum=-1;
				try {
					filterNum = std::stoi(s.substr(pos + 1));
				}
				catch (std::exception e) {continue;}
				it++; //the nextline should be numerator
				s = *it;
				if (s.find("Numerator Length:") == std::string::npos) {
					continue; //if it isnt skip
				}
				//find the colon again
				pos = s.find(":");
				int Numeratorcount = -1;
				try {
					Numeratorcount = std::stoi(s.substr(pos + 1));
				}
				catch (std::exception e) {continue;}
				//write back
				if (filterNum != -1 && Numeratorcount > 0) {
					(*coeffCount_Num)[i][(size_t)filterNum-1] = Numeratorcount;
				}
				else {
					continue;
				}
				//skip the numerator lines
				it +=(size_t)Numeratorcount+1;
				s = *it; //this should be the denominator count
				if (s.find("Denominator Length:") == std::string::npos) {
					continue;
				}
				//find colon
				pos = s.find(":");
				int denominatorcount = -1;
				try {
					denominatorcount = stoi(s.substr(pos + 1));
				}
				catch (std::exception e) { continue; }
				if (filterNum != -1 && denominatorcount > 0) {
					(*coeffCount_Denom)[i][(size_t)filterNum - 1] = denominatorcount;
				}
				else { continue; }
			}
		}
	}//we got all the numerator and denominator counts

	 //check for maxCoefficientCount
	int maxCoeffCount = -1;
	for (int i = 0; i < filterAmount; i++) {
		for (int j = 0; j < filterAmount; j++) {
			if (j <= i) {
				if ((*coeffCount_Num)[i][j] > maxCoeffCount) {
					maxCoeffCount = (*coeffCount_Num)[i][j];
				}
				if ((*coeffCount_Denom)[i][j] > maxCoeffCount) {
					maxCoeffCount = (*coeffCount_Denom)[i][j];
				}
			}
		}
	}
	//check the maxCoefficientcount
	if (maxCoeffCount < 1) {
		std::cout << "files are not in correct format";
		exit(EXIT_FAILURE);
	}
	
	
	//resize the 3D-Structurezero it
	(*allCoeffs_Num).resize(filterAmount);
	(*allCoeffs_Denom).resize(filterAmount);
	for (int i = 0; i < filterAmount; i++) {
		(*allCoeffs_Num)[i].resize(filterAmount);
		(*allCoeffs_Denom)[i].resize(filterAmount);
		for (int j = 0; j < filterAmount; j++) {
			(*allCoeffs_Num)[i][j].resize(maxAmountCoeffs);
			(*allCoeffs_Denom)[i][j].resize(maxAmountCoeffs);
			for (int k = 0; k < maxAmountCoeffs;k++) {
				(*allCoeffs_Num)[i][j][k] = 0;
				(*allCoeffs_Denom)[i][j][k] = 0;
			}
		}
	}
	
	//read all files again
	for (int i = 0; i < filterAmount; i++) {
		std::ifstream file;
		file.open(filepaths[i]);
		if (!file) { //file cant be opened, we already checked this so skip this
			file.close();
			continue;
		}
		//we opened the file successfully
		filecontent.clear();
		while (!file.eof()) {
			std::string s;
			std::getline(file, s);
			filecontent.push_back(s);
		}
		file.close();
		for (std::vector<std::string>::iterator it = filecontent.begin(); it != filecontent.end(); it++) {
			std::string s = *it;
			//erase whitespaces
			s.erase(std::remove_if(s.begin(), s.end(), ::isspace), s.end());
			//check if we get something like "Filterband:"
			if (s.find("Filterband:") != std::string::npos) {
				size_t pos = s.find(":");
				int filterNum = -1;
				try {
					filterNum = std::stoi(s.substr(pos + 1));
				}
				catch (std::exception e) { continue; }
				if (filterNum == -1) { continue; }
				int CoeffsNum_num = (*coeffCount_Num)[i][(size_t)filterNum - 1];
				int CoeffsDenom_num = (*coeffCount_Denom)[i][(size_t)filterNum - 1];
				it++;//next line should be numerator count
				it++; //this is the first numerator
				for (int j = 0; j < CoeffsNum_num; j++, it++) {
					s = *it;
					try {
						(*allCoeffs_Num)[i][(size_t)filterNum - 1][j] = std::stod(s);
					}
					catch (std::exception e) { continue; }
				}
				//current line should be denominator count
				it++;//this is the first denominator
				for (int j = 0; j < CoeffsDenom_num; j++, it++) {
					s = *it;
					try {
						if (j == 0) {
							(*denom0Coeff)[i][(size_t)filterNum - 1] = std::stod(s);
						}
						else {
							(*allCoeffs_Denom)[i][(size_t)filterNum - 1][(size_t)j - 1] = std::stod(s);
						}
					}
					catch (std::exception e) { continue; }
				}
				//std::cout << *it;
			}
		}//all coeffs read
	}
}
//params:
//numCoeffsLoPass: pointer to Variable for lowpasscoeffAmount rounded to the next upper multiple of 8
//numCoeffsLo256bufs: pointer to Variable for amount of 256buffers to be used basically numCoeffsLoPass/8
//numOfNum: pointer to variable for the amount of Numerators
//numOfDenom: pointer to variable for the amount of Denominators
//numCoeffs: pointer to vector for Numerators
//DenomCoeffs: pointer to vector for Denumerators
//Denom0; pointer for the 0 Denominator
void ReadLowPassForEnvelope(int  numCoeffsLoPass, int  numCoeffsLo256bufs, int * numOfNum, int * numOfDenom, std::vector<double> * NumCoeffs, std::vector<double> *DenomCoeffs, double* Denom0) {
	try {
		rootfolder = configurationMap.at("FILTERLOCATION") + "/";
	}
	catch (std::exception e) {}
	int NumeratorCount=-1;
	int DenominatorCount=-1;
	std::string filename = rootfolder + "lowpass_for_envelope.txt";
	std::ifstream file;
	file.open(filename);
	if (!file) {
		std::cout << "could not find filter for envelope, does it exist?\n";
		file.close();
		exit(EXIT_FAILURE);
	}
	filecontent.clear();
	while (!file.eof()) {
		std::string s;
		std::getline(file, s);
		filecontent.push_back(s);
	}
	file.close();
	//file is read
	for (std::vector<std::string>::iterator it = filecontent.begin(); it != filecontent.end(); it++) {
		//check for filterlength again
		//check if we get Numerator Length
		std::string s = *it;
		if (s.find("Numerator Length:") != std::string::npos) {
			//erase whitespaces
			s.erase(std::remove_if(s.begin(), s.end(), ::isspace), s.end());
			size_t pos = s.find(":"); //find the colon the number is just behind it
			s = s.substr(pos + 1);
			try {
				int c = std::stoi(s);
				NumeratorCount = c;
			}
			catch (std::exception e) {} //skip exceptions
		}//we got the numerator count
		s = *it;
		if (s.find("Denominator Length:") != std::string::npos) {
			//same as before
			s.erase(std::remove_if(s.begin(), s.end(), ::isspace), s.end());
			size_t pos = s.find(":"); //find the colon the number is just behind it
			s = s.substr(pos + 1);
			try {
				int c = std::stoi(s);
				DenominatorCount = c;
			}
			catch (std::exception e) {} //skip exceptions
		}//we got the denominator count
	}
	//int x = DenominatorCount > NumeratorCount ? DenominatorCount : NumeratorCount;
	//set to 8
	//int x = 8;
	//int y = std::ceil((float)x / 8.0);
	//*numCoeffsLo256bufs = y;
	//*numCoeffsLoPass = y * 8;
	*numOfNum = NumeratorCount;
	*numOfDenom = DenominatorCount;
	///std::cout << NumeratorCount << "NUMERATOR\nDENOMINATOR" << DenominatorCount << "\n";
	//std::cout << (y / 8) << "\n";

	//resize
	(*NumCoeffs).resize(numCoeffsLoPass);
	(*DenomCoeffs).resize(numCoeffsLoPass);
	for (int i = 0; i < numCoeffsLoPass; i++) {
		(*NumCoeffs)[i] = 0;
		(*DenomCoeffs)[i] = 0;
	}


	//read everything again

	for (std::vector<std::string>::iterator it = filecontent.begin(); it != filecontent.end(); it++) {
		std::string s = *it;
		//std::cout << s<<"\n";
		if (s.find("Numerator Length:") != std::string::npos) { //we found the start of the numerators
			//std::cout << s << "\n";
			if (NumeratorCount > 0) {
				it++;//go to next line
				for (int k = 0; k < NumeratorCount; k++, it++) {
					try {
						double coeff = std::stod(*it);
						//put it into the 3DVector
						(*NumCoeffs)[k] = coeff;
					}
					catch (std::exception e) {}
				}
			}
		}
		s = *it;
		if (s.find("Denominator Length:") != std::string::npos) {
			//std::cout << "Do we even get here?";
			if (DenominatorCount > 0) {
				it++;
				for (int k = 0; k < DenominatorCount; k++, it++) {
					try {
						double coeff = std::stod(*it);
						if (k == 0) {
							*Denom0 = coeff;
						}
						else {
							(*DenomCoeffs)[(size_t)k - 1] = coeff;
						}
					}
					catch (std::exception e) {}
				}
			}
		}

	}//all filtercoeffs read
};

void ReadCenterFrequencies(int NumOfFilters, std::vector<std::vector<float>>* freqs) {
	try {
		rootfolder = configurationMap.at("FILTERLOCATION") + "/";
	}
	catch (std::exception e) {}
	//init the frequenciesVector and zero it
	(*freqs).resize(NumOfFilters);
	for (int i = 0; i < NumOfFilters; i++) {
		(*freqs)[i].resize(NumOfFilters);
		for (int j = 0; j < NumOfFilters; j++) {
			(*freqs)[i][j] = 0;
		}
	}
	for (int i = 0; i < NumOfFilters; i++) {
		std::ifstream file;
		file.open(filepaths[i]);
		if (!file) {
			file.close();
			exit(EXIT_FAILURE);
		}
		std::string s;
		filecontent.clear();
		while (!file.eof()) {
			std::getline(file, s);
			filecontent.push_back(s);
		}
		file.close();
		//file is read
		for (std::vector<std::string>::iterator it = filecontent.begin(); it != filecontent.end(); it++) {
			std::string s = *it;
			s.erase(std::remove_if(s.begin(), s.end(), ::isspace), s.end());
			//find "CENTERFREQUENCIES:"
			if (s.find("CENTERFREQUENCIES:") != std::string::npos) {
				it++; //freqs should start right below
				for (int j = 0; j <= i; j++, it++) {
					s = *it;
					try {
						(*freqs)[i][j] = std::stof(s);
						//std::cout << (*freqs)[i][j] << "\n";
					}
					catch (std::exception e) { continue; }
				}
			}
		}
	}
};

void ReadFilterWeights(int FilterAmount, std::vector<std::vector<float>>* filterweights) {
	try {
		rootfolder = configurationMap.at("FILTERLOCATION") + "/";
	}
	catch (std::exception e) {}
	//set up 2dVector
	(*filterweights).resize(FilterAmount);
	for (int i = 0; i < FilterAmount; i++) {
		(*filterweights)[i].resize(FilterAmount);
		for (int j = 0; j < FilterAmount; j++) {
			(*filterweights)[i][j] = 1;
		}
	}
	std::string filename = rootfolder + "filterweights.txt";
	std::ifstream file;
	file.open(filename);
	if (!file) {
		file.close();
		std::cout << "filterweights could not be opened.\n";
	}
	std::string s;
	filecontent.clear();
	while (!file.eof()) {
		std::getline(file, s);
		filecontent.push_back(s);
	}
	file.close();
	//file read
	for (std::vector<std::string>::iterator it = filecontent.begin(); it != filecontent.end(); it++) {
		std::string s = *it;
		s.erase(std::remove_if(s.begin(), s.end(), ::isspace), s.end());
		size_t pos = s.find("FilterBank");
		size_t poscolon = s.find(":");
		if (pos == std::string::npos||poscolon ==std::string::npos) {
			continue;
		}
		int filterbank=0;
		try {
			filterbank = std::stoi(s.substr(pos+10, poscolon - (pos+10)));
			//std::cout << filterbank;
		}
		catch (std::exception e) {
			continue;
		}
		it++;
		for (int i = 0; i < filterbank; i++) {
			s = *it;
			try {
				float weight = std::stof(s);
				if (weight >= 0 && weight <= 1) {
					(*filterweights)[(size_t)filterbank - 1][i] = weight;
					//std::cout << filterbank << "_" << i << ":" << weight << "\n";
				}
				if (i < filterbank - 1) {
					it++;
				}
				
			}
			catch (std::exception e) {
				continue;
			}
		}
	}
}
