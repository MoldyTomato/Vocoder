/*
This file contains the fun stuff: the vocoding functions, should be called from the main processing callback from ASIOFuntions.h

//funtions here
void printLowPassCoefficients()		prints the coefficients for the lowpassfilter
void printCenterFrequecies()		prints the centerfrequencies used for the sine waves
void printCoefficientsPerFilter()	prints the number of coefficients per filter
void printAllFilterCoefficients()	prints all coefficients for the bandpassfilters
void createSineWave()				creates a samplesinewave with f = 1Hz
void createNoise()					creates a noisevector to sample from
void createSquare()
void createSaw()
void setChannel(int i)				sets the channelamount used
float hsum256(__m256 v)				computes the sum of a __m256 vector (horizontal)
double hsum256_pd(__m256d v)		computes the sum of a __m256d vector (horizontal)
void extract_256(__m256 v, float* a, float* b, float* c, float* d, float* e, float* f, float* g, float* h)		extracts the floats from a __m256 vector to the given locations
void print_256(__m256 v)			prints a __m256 Vector
void print_256_pd(__m256d v)		prints a __m256d Vector
void vocode_fast()					vocodes the input in SignalBufferIn, fast but not accurate
void vocode_accurate()				vocoder same as vocode_fast() but accurate :)
void performTest()					performs some test runs to see if the routine is fast enough
void setupForVocoding(int buffSize)	needs to be called before actually vocoding, sets up all the necessary things by reading filters filling and zeroeing the AVX-Buffers
		!dont forget to actually assign the vocoding function you want to use to vocode() see: line 1078
void resetBufferValues()			resets the buffervalues
*/


#pragma once

//includes
#include <vector>
#include <iostream>
#include <immintrin.h>
#include <iomanip>
#include <algorithm>
#include <chrono>


//includes own stuff
#include "fcfFileReader.h"
#include "VocoderStatus.h"
#include <random>

//the vocoder function
void (*vocode)();


//externals
extern std::vector<float> SignalBufferIn; //the incoming Buffer from AsioCallback
extern std::vector<float> SignalBufferOut; //the outgoing Buffer to AsioCallback
extern VocoderStatus* currentVocoderState;
////////////////////////////////////////
//incoming values
__m256d previousInputValues256_0, previousInputValues256_1; //we need 2 since we're working with doubles and max 8 coefficients
__m256d previousNoiseInputValues256_0, previousNoiseInputValues256_1;
//some vars for calcs
__m256d l2f, lolo, blend, hihi, in;
__m128d lo, hi;



//
//filtered Values
double filteredValue; //the filtered Value at the end of a filter
double filteredNoiseValue; //the filtered noise Value at the end of a filter
double postfilteredValue;


//Bandpasses
std::vector<std::vector<std::vector<double>>> ALLFILTERNUMERATORCOEFFICIENTS; //will hold all the numerators of all filters
std::vector<std::vector<std::vector<double>>> ALLFILTERDENOMINATORCOEFFICIENTS; //will hold all the denominators of all filters, starting form [1,N]
int MAXNUMOFBANDPASSBANDS; //Max Amount of Bandpasses the Vocoder uses
std::vector<std::vector<int>> NUMOFNUMERATORCOEFFICIENTSPERFILTER; //will hold the Number of numerators per filter
std::vector<std::vector<int>> NUMOFDENOMINATORCOEFFICIENTSPERFILTER; //will hold the Number of denominators per filter
std::vector<std::vector<double>> DENOMINATOR0COEFFICIENT; //Holds the Denominator at index 0, usually 1.0 but other values can occur

std::vector<std::vector<__m256d>> ALLBANDPASSFILTERNUMERATORCOEFFICIENTS256_0; 
std::vector<std::vector<__m256d>> ALLBANDPASSFILTERNUMERATORCOEFFICIENTS256_1; //all numerators in 256buffers
std::vector<std::vector<__m256d>> ALLBANDPASSFILTERDENOMINATORCOEFFICIENTS256_0; //all denominators in 256buffers
std::vector<std::vector<__m256d>> ALLBANDPASSFILTERDENOMINATORCOEFFICIENTS256_1; //all denominators in 256buffers

std::vector<__m256d> previousBandpassOutputValues256_0, previousBandpassOutputValues256_1;
std::vector<__m256d> previousBandpassNoiseOutputValues256_0, previousBandpassNoiseOutputValues256_1;


//Lowpass
__m256d LOWPASSFILTERNUMERATORCOEFFICIENTS256_0, LOWPASSFILTERNUMERATORCOEFFICIENTS256_1;
__m256d LOWPASSFILTERDENOMINATORCOEFFICIENTS256_0, LOWPASSFILTERDENOMINATORCOEFFICIENTS256_1;
double LOWPASSDENOMINATOR0 = 1;
int MAXNUMOFLOWPASSCOEFFICIENTS; //hold the max amount of lowpassfiltercoefficients
int NUMOF256BUFFERFORLOWPASSCOEFFICIENTS;
int NUMOFLOWPASSNUMERATORCOEFFICIENTS;
int NUMOFLOWPASSDENOMINATORCOEFFICIENTS;
std::vector<double> LOWPASSFILTERNUMERATORCOEFFICIENTS; //will hold the Numerators for the lowpassfilter for envelope extraction
std::vector<double> LOWPASSFILTERDENOMINATORCOEFFICIENTS; //will hold the Denominators for the lowpassfilter for envelope extraction
std::vector<__m256d> previousLowpassFilterInputValues256_0, previousLowpassFilterInputValues256_1;
std::vector <__m256d> previousLowpassFilterOutputValues256_0, previousLowpassFilterOutputValues256_1; //for envelope extraction


//Filterweights
std::vector<std::vector<float>> FILTERWEIGHTS;

//postfilter
std::vector<__m256d> previousPostFilterInputValues_0, previousPostFilterInputValues_1;
std::vector<__m256d> previousPostFilterOutputValues_0, previousPostFilterOutputValues_1;


//CAPITALS WONT BE CHANGED AFTER THE VOCODER IS RUNNING
int BUFFERSIZE;




bool DIVISIONBYDENOMINATOR0 = false; //Indicates wether we do a division by the first Denominator

int NUMOF256BUFFERSFORCOEFFICIENTS; //for bandpasses
int MAXNUMOFCOEFFICIENTS; //for bandpasses

//CARRIER
std::vector<double> SINEWAVEVECTOR;
std::vector<int> sinepointer; //use only MAXNUMBANDPASS sinepointers
std::vector<double> NOISEVECTOR;
int noisepointer;
std::vector<double> SAWWAVEVECTOR; //uses the same pointers as sine
std::vector<double> SQUAREWAVEVECTOR; //uses the same pointers as sine
//centerfrequencies
std::vector<std::vector<float>> CENTERFREQUENCIES;


//will hold the current number of filterbands
int currentAmountOfBandpasses = 1; //starts with 1!!

//prints lowpass filtercoefficients
void printLowPassCoefficients() {
	std::cout << "Numerators of Lowpass\n";
	for (int i = 0; i < NUMOFLOWPASSNUMERATORCOEFFICIENTS; i++) {
		std::cout << std::setprecision(15)<<LOWPASSFILTERNUMERATORCOEFFICIENTS[i] << "\n";
	}
	std::cout << "Denominators of Lowpass\n";
	for (int i = 0; i < NUMOFLOWPASSDENOMINATORCOEFFICIENTS; i++) {
		std::cout << std::setprecision(15) << LOWPASSFILTERDENOMINATORCOEFFICIENTS[i] << "\n";
	}
};
//prints Centerfrequencies
void printCenterFrequecies() {
	std::cout << "Centerfrequencies:\n";
	for (int i = 0; i < MAXNUMOFBANDPASSBANDS; i++) {
		for (int j = 0; j < MAXNUMOFBANDPASSBANDS; j++) {
			if (j <= i) {
				std::cout << "[" << CENTERFREQUENCIES[i][j] << "Hz]";
			}
		}
		std::cout << "\n";
	}
};
//prints the Amount of Filtercoefficients, for debugging purposes only, dont call after the according vector is destroyed
void printCoefficientsPerFilter() {
	std::cout << "Numerators per Filter\n";
	for (int i = 0; i < MAXNUMOFBANDPASSBANDS; i++) {
		for (int j = 0; j < MAXNUMOFBANDPASSBANDS; j++) {
			if (j <= i) {
				std::cout << "[" << NUMOFNUMERATORCOEFFICIENTSPERFILTER[i][j] << "]";
			}
		}
		std::cout << "\n";
	}
	std::cout << "Denominators per Filter\n";
	for (int i = 0; i < MAXNUMOFBANDPASSBANDS; i++) {
		for (int j = 0; j < MAXNUMOFBANDPASSBANDS; j++) {
			if (j <= i) {
				std::cout << "[" << NUMOFDENOMINATORCOEFFICIENTSPERFILTER[i][j] << "]";
			}
		}
		std::cout << "\n";
	}
};
//prints Filtercoefficients, for debugging purposes only, dont call after the according vector is destroyed
void printAllFilterCoefficients() {
	std::cout << "All Numeratorfiltercoefficients:\n";
	for (int i = 0; i < MAXNUMOFBANDPASSBANDS; i++) {
		for (int j = 0; j < MAXNUMOFBANDPASSBANDS; j++) {
			if (j <= i) {
				std::cout << "[" << i << "]" << "[" << j << "]:";
				for (int k = 0; k < NUMOFNUMERATORCOEFFICIENTSPERFILTER[i][j]; k++) {
					std::cout << std::fixed << std::setprecision(15) << ALLFILTERNUMERATORCOEFFICIENTS[i][j][k] << ",";
				}
				std::cout << "\n";
			}
		}
		std::cout << "\n";
	}
	std::cout << "All Denominatorfiltercoefficients:\n";
	for (int i = 0; i < MAXNUMOFBANDPASSBANDS; i++) {
		for (int j = 0; j < MAXNUMOFBANDPASSBANDS; j++) {
			if (j <= i) {
				std::cout << "[" << i << "]" << "[" << j << "]:";
				for (int k = 0; k < NUMOFDENOMINATORCOEFFICIENTSPERFILTER[i][j]; k++) {
					std::cout << std::fixed << std::setprecision(15) << ALLFILTERDENOMINATORCOEFFICIENTS[i][j][k] << ",";
				}
				std::cout << "\n";
			}
		}
		std::cout << "\n";
	}
};




//creates a sine wave
void createSineWave() {
	SINEWAVEVECTOR.resize(44100);
	double pi = 2 * std::acos(0.0);
	for (int i = 0; i < 44100; i++) {
		SINEWAVEVECTOR[i] = std::sin(2.0 * i * pi / 44100)*0.9; //dont go full to +/-1 otherwise it clips
	}
};
//creates 44100 white-noise samples which will be looped
void createNoise() {
	NOISEVECTOR.resize(44100);
	std::default_random_engine generator;
	std::uniform_real_distribution<double> distribution(-1.0, 1.0);
		//		<double> distribution(0.0, 0.4);
	//float m = 2.0;
	for (int i = 0; i < 44100; i++) {
		//float x = (float(rand()) / float(RAND_MAX)) * m;
		//NOISEVECTOR[i] = x - m/2.0*0.9; //same here dont go to +/-1
		double x = distribution(generator)*0.9;
		//std::cout << x << "\n";
		assert(x > -1.0 && x < 1.0);
		NOISEVECTOR[i] = x;
	}
};


//creates a squarewave, just for fun
void createSquare() {
	SQUAREWAVEVECTOR.resize(44100);
	for (int i = 0; i < 44100; i++) {
		double x;
		if (i < 44100 / 2) {
			x = 1;
		}
		else {
			x = -1;
		}
		SQUAREWAVEVECTOR[i] = x*0.9;//not full to +/-1
	}
}
//creates a sawwave, just for fun
void createSaw() {
	SAWWAVEVECTOR.resize(44100);
	for (int i = 0; i < 44100; i++) {
		double x = (double)i / 22050 - 1.0;
		SAWWAVEVECTOR[i] = x*0.9;
	}
}

//checks if the given Parameter is viable and sets it, will switch immediatly
void setChannel(int i) {
	if (i > 0 && i <= MAXNUMOFBANDPASSBANDS) {
		currentAmountOfBandpasses = i;
		//std::cout << "Channel set to "<<currentAmountOfBandpasses<<"\n";
	}
};


//Computes the sum of one __m256 vector, couldnt find a faster way to do the horizontal sum
float hsum256(__m256 v) {
	float x = 0;
	//horizantal sum (a,b,c,d,e,f,g,h),(a,b,c,d,e,f,g,h)->(a+b,c+d,a+b,c+d,e+f,g+h,e+f,g+h)
	__m256 sum = _mm256_hadd_ps(v, v);
	//extract low half ->(a+b,c+d,a+b,c+d)
	__m128 low = _mm256_extractf128_ps(sum, 0);
	//extract high half ->(e+f,g+h,e+f,g+h)
	__m128 high = _mm256_extractf128_ps(sum, 1);
	//insert low halves ->(a+b,c+d,a+b,c+d,a+b,c+d,a+b,c+d)
	__m256 bothlow = _mm256_set_m128(low, low);
	//insert high halves ->(e+f,g+h,e+f,g+h,e+f,g+h,e+f,g+h)
	__m256 bothhigh = _mm256_set_m128(high, high);
	//get first value ->(a+b)
	x += _mm256_cvtss_f32(bothlow);
	//shift ->(c+d,a+b,c+d,a+b,c+d,a+b,c+d,a+b)
	bothlow = _mm256_permute_ps(bothlow, 0x39);
	//get first value ->(c+d)
	x += _mm256_cvtss_f32(bothlow);
	//get first value ->(e+f)
	x += _mm256_cvtss_f32(bothhigh);
	//shift ->(g+h,e+f,g+h,e+f,g+h,e+f,g+h,e+f)
	bothhigh = _mm256_permute_ps(bothhigh, 0x39);
	//get first value ->(g+h)
	x += _mm256_cvtss_f32(bothhigh);
	return x;
}

double hsum256_pd(__m256d v) {
	__m256d s = v;
	double x = 0;
	x+= _mm256_cvtsd_f64(s);
	s = _mm256_permute_pd(s, 0x5);
	x+= _mm256_cvtsd_f64(s);
	s = v;
	__m128d high = _mm256_extractf128_pd(s, 1);
	s = _mm256_set_m128d(high, high);
	x+= _mm256_cvtsd_f64(s);
	s = _mm256_permute_pd(s, 0x5);
	x+=_mm256_cvtsd_f64(s);
	return x;
}


//extracts a 256 buffers and puts the float into the given places
void extract_256(__m256 v, float* a, float* b, float* c, float* d, float* e, float* f, float* g, float* h) {
	__m256 s = v;
	*a = _mm256_cvtss_f32(s);
	s = _mm256_permute_ps(s, 0x39);
	*b = _mm256_cvtss_f32(s);
	s = _mm256_permute_ps(s, 0x39);
	*c = _mm256_cvtss_f32(s);
	s = _mm256_permute_ps(s, 0x39);
	*d = _mm256_cvtss_f32(s);
	s = _mm256_permute_ps(s, 0x39);
	__m128 high = _mm256_extractf128_ps(s, 1);
	s = _mm256_set_m128(high, high);
	*e = _mm256_cvtss_f32(s);
	s = _mm256_permute_ps(s, 0x39);
	*f = _mm256_cvtss_f32(s);
	s = _mm256_permute_ps(s, 0x39);
	*g = _mm256_cvtss_f32(s);
	s = _mm256_permute_ps(s, 0x39);
	*h = _mm256_cvtss_f32(s);
};

//prints a __m256 vector
void print_256(__m256 v) {
	__m256 s = v;
	std::cout<<_mm256_cvtss_f32(s)<<",";
	s = _mm256_permute_ps(s, 0x39);
	std::cout <<  _mm256_cvtss_f32(s)<<",";
	s = _mm256_permute_ps(s, 0x39);
	std::cout << _mm256_cvtss_f32(s)<<",";
	s = _mm256_permute_ps(s, 0x39);
	std::cout << _mm256_cvtss_f32(s)<<",";
	s = _mm256_permute_ps(s, 0x39);
	__m128 high = _mm256_extractf128_ps(s, 1);
	s = _mm256_set_m128(high, high);
	std::cout << _mm256_cvtss_f32(s)<<",";
	s = _mm256_permute_ps(s, 0x39);
	std::cout << _mm256_cvtss_f32(s)<<",";
	s = _mm256_permute_ps(s, 0x39);
	std::cout << _mm256_cvtss_f32(s)<<",";
	s = _mm256_permute_ps(s, 0x39);
	std::cout << _mm256_cvtss_f32(s)<<"\n";
};
void print_256_pd(__m256d v) {
	__m256d s = v;
	std::cout << _mm256_cvtsd_f64(s)<<",";
	s = _mm256_permute_pd(s, 0x5);
	std::cout<< _mm256_cvtsd_f64(s)<<",";
	s = v;
	__m128d high = _mm256_extractf128_pd(s, 1);
	s = _mm256_set_m128d(high, high);
	std::cout << _mm256_cvtsd_f64(s)<<",";
	s = _mm256_permute_pd(s, 0x5);
	std::cout << _mm256_cvtsd_f64(s)<<"\n";
}

//this function does the whole vocoding, simplified for perfomance
void vocode_fast() {

	//for each inputvalue
	for (int i = 0; i < BUFFERSIZE; i++) {

		//push in the incoming value
		l2f = _mm256_permute_pd(previousInputValues256_1, 0x5);
		lo = _mm256_extractf128_pd(l2f,0);
		lolo = _mm256_set_m128d(lo, lo);
		blend = _mm256_blend_pd(l2f, lolo, 0x4);
		l2f = _mm256_permute_pd(previousInputValues256_0, 0x5);
		hi = _mm256_extractf128_pd(l2f, 1);
		hihi = _mm256_set_m128d(hi, hi);
		previousInputValues256_1 = _mm256_blend_pd(blend, hihi, 0x1);
			//first buffer done
		lo = _mm256_extractf128_pd(l2f, 0);
		lolo = _mm256_set_m128d(lo, lo);
		blend = _mm256_blend_pd(l2f, lolo, 0x5);
		in = _mm256_set1_pd((double)SignalBufferIn[i]);
		previousInputValues256_0 = _mm256_blend_pd(blend, in, 0x1);
			//second buffer done


		//we should actuall push always in either noise or zeros but for performance reasons we push only we we actually use it and we wont really notice it
		if (currentVocoderState->carrier == NOISE) {
			//push in the noise
			l2f = _mm256_permute_pd(previousNoiseInputValues256_1, 0x5);
			lo = _mm256_extractf128_pd(l2f, 0);
			lolo = _mm256_set_m128d(lo, lo);
			blend = _mm256_blend_pd(l2f, lolo, 0x4);
			l2f = _mm256_permute_pd(previousNoiseInputValues256_0, 0x5);
			hi = _mm256_extractf128_pd(l2f, 1);
			hihi = _mm256_set_m128d(hi, hi);
			previousNoiseInputValues256_1 = _mm256_blend_pd(blend, hihi, 0x1);
			//first buffer done
			lo = _mm256_extractf128_pd(l2f, 0);
			lolo = _mm256_set_m128d(lo, lo);
			blend = _mm256_blend_pd(l2f, lolo, 0x5);
			in = _mm256_setzero_pd();
			in = _mm256_set1_pd(NOISEVECTOR[noisepointer]);
			noisepointer++; noisepointer %= 44100;
			previousNoiseInputValues256_0 = _mm256_blend_pd(blend, in, 0x1);
			//second buffer done
		}

		//zero Outvalue
		SignalBufferOut[i] = 0;

		//filter the value for each band
		for (int filterband = 0; filterband < currentAmountOfBandpasses; filterband++) {
			filteredValue = 0;
			filteredNoiseValue = 0;
			//mult add
			filteredValue = hsum256_pd(
				_mm256_sub_pd(
					_mm256_add_pd(
						_mm256_mul_pd(ALLBANDPASSFILTERNUMERATORCOEFFICIENTS256_0[currentAmountOfBandpasses - 1][filterband], previousInputValues256_0),
						_mm256_mul_pd(ALLBANDPASSFILTERNUMERATORCOEFFICIENTS256_1[currentAmountOfBandpasses - 1][filterband], previousInputValues256_1)
					),
					_mm256_add_pd(
						_mm256_mul_pd(ALLBANDPASSFILTERDENOMINATORCOEFFICIENTS256_0[currentAmountOfBandpasses - 1][filterband], previousBandpassOutputValues256_0[filterband]),
						_mm256_mul_pd(ALLBANDPASSFILTERDENOMINATORCOEFFICIENTS256_1[currentAmountOfBandpasses - 1][filterband], previousBandpassOutputValues256_1[filterband])
					)
				)
			);
			//filter Noise if needed
			if (currentVocoderState->carrier == NOISE) {
				filteredNoiseValue = hsum256_pd(
					_mm256_sub_pd(
						_mm256_add_pd(
							_mm256_mul_pd(ALLBANDPASSFILTERNUMERATORCOEFFICIENTS256_0[currentAmountOfBandpasses - 1][filterband], previousNoiseInputValues256_0),
							_mm256_mul_pd(ALLBANDPASSFILTERNUMERATORCOEFFICIENTS256_1[currentAmountOfBandpasses - 1][filterband], previousNoiseInputValues256_1)
						),
						_mm256_add_pd(
							_mm256_mul_pd(ALLBANDPASSFILTERDENOMINATORCOEFFICIENTS256_0[currentAmountOfBandpasses - 1][filterband], previousBandpassNoiseOutputValues256_0[filterband]),
							_mm256_mul_pd(ALLBANDPASSFILTERDENOMINATORCOEFFICIENTS256_1[currentAmountOfBandpasses - 1][filterband], previousBandpassNoiseOutputValues256_1[filterband])
						)
					)
				);
			}


			//check if we need to divide you can uncomment this if you need it, but usually the first denominator is 1
			/*
			if (DIVISIONBYDENOMINATOR0) {
				filteredValue = filteredValue / DENOMINATOR0COEFFICIENT[currentAmountOfBandpasses - 1][filterband];
				filteredNoiseValue = filteredNoiseValue / DENOMINATOR0COEFFICIENT[currentAmountOfBandpasses - 1][filterband];
			}
			*/
			/*
			if (isnan(filteredValue)) {
				std::cout << "bandpassfilter: " << filterband << " seems to be unstable\n";
			}

			if (isnan(filteredNoiseValue)) {
				std::cout << "bandpassfilter : "<<filterband<<" seems to be unstable for noise\n";
			}
			*/

			
			//pushback the filtered value
			l2f = _mm256_permute_pd(previousBandpassOutputValues256_1[filterband], 0x5);
			lo = _mm256_extractf128_pd(l2f, 0);
			lolo = _mm256_set_m128d(lo, lo);
			blend = _mm256_blend_pd(l2f, lolo, 0x4);
			l2f = _mm256_permute_pd(previousBandpassOutputValues256_0[filterband], 0x5);
			hi = _mm256_extractf128_pd(l2f, 1);
			hihi = _mm256_set_m128d(hi, hi);
			previousBandpassOutputValues256_1[filterband] = _mm256_blend_pd(blend, hihi, 0x1);
			//first buffer done
			lo = _mm256_extractf128_pd(l2f, 0);
			lolo = _mm256_set_m128d(lo, lo);
			blend = _mm256_blend_pd(l2f, lolo, 0x5);
			in = _mm256_set1_pd((double)filteredValue);
			previousBandpassOutputValues256_0[filterband] = _mm256_blend_pd(blend, in, 0x1);
			//second buffer done
			
			//only push back if we actually filtered something for perfomance reasons
				if (currentVocoderState->carrier == NOISE) {
					//push back the filtered Noise
					//pushback the filtered value
					l2f = _mm256_permute_pd(previousBandpassNoiseOutputValues256_1[filterband], 0x5);
					lo = _mm256_extractf128_pd(l2f, 0);
					lolo = _mm256_set_m128d(lo, lo);
					blend = _mm256_blend_pd(l2f, lolo, 0x4);
					l2f = _mm256_permute_pd(previousBandpassNoiseOutputValues256_0[filterband], 0x5);
					hi = _mm256_extractf128_pd(l2f, 1);
					hihi = _mm256_set_m128d(hi, hi);
					previousBandpassNoiseOutputValues256_1[filterband] = _mm256_blend_pd(blend, hihi, 0x1);
					//first buffer done
					lo = _mm256_extractf128_pd(l2f, 0);
					lolo = _mm256_set_m128d(lo, lo);
					blend = _mm256_blend_pd(l2f, lolo, 0x5);
					in = _mm256_set1_pd(filteredNoiseValue);
					previousBandpassNoiseOutputValues256_0[filterband] = _mm256_blend_pd(blend, in, 0x1);
				}

			
			//lowpassfilter the value
			//use the absolute outputvalue of the bandpass so no need to pushback the incoming value
			filteredValue = hsum256_pd(
				_mm256_sub_pd(
					_mm256_add_pd(
						_mm256_mul_pd(LOWPASSFILTERNUMERATORCOEFFICIENTS256_0, 
							_mm256_sqrt_pd(_mm256_mul_pd(previousBandpassOutputValues256_0[filterband], previousBandpassOutputValues256_0[filterband]))
						),
						_mm256_mul_pd(LOWPASSFILTERNUMERATORCOEFFICIENTS256_1, 
							_mm256_sqrt_pd(_mm256_mul_pd(previousBandpassOutputValues256_1[filterband], previousBandpassOutputValues256_1[filterband]))
						)
					),
					_mm256_add_pd(
						_mm256_mul_pd(LOWPASSFILTERDENOMINATORCOEFFICIENTS256_0, previousLowpassFilterOutputValues256_0[filterband]),
						_mm256_mul_pd(LOWPASSFILTERDENOMINATORCOEFFICIENTS256_1, previousLowpassFilterOutputValues256_1[filterband])
					)
				)
			);

			//divide if needed, uncomment this if you need it
			/*
			if (LOWPASSDENOMINATOR0 != 1) {
				filteredValue = filteredValue / LOWPASSDENOMINATOR0;
			}
			*/
			/*
			if (isnan(filteredValue)) {
				std::cout << "lowpassfilter seems to be unstable\n";
			}
			*/

			//push back the outputvalue
			l2f = _mm256_permute_pd(previousLowpassFilterOutputValues256_1[filterband], 0x5);
			lo = _mm256_extractf128_pd(l2f, 0);
			lolo = _mm256_set_m128d(lo, lo);
			blend = _mm256_blend_pd(l2f, lolo, 0x4);
			l2f = _mm256_permute_pd(previousLowpassFilterOutputValues256_0[filterband], 0x5);
			hi = _mm256_extractf128_pd(l2f, 1);
			hihi = _mm256_set_m128d(hi, hi);
			previousLowpassFilterOutputValues256_1[filterband] = _mm256_blend_pd(blend, hihi, 0x1);
			//first buffer done
			lo = _mm256_extractf128_pd(l2f, 0);
			lolo = _mm256_set_m128d(lo, lo);
			blend = _mm256_blend_pd(l2f, lolo, 0x5);
			in = _mm256_set1_pd((double)filteredValue);
			previousLowpassFilterOutputValues256_0[filterband] = _mm256_blend_pd(blend, in, 0x1);
			//second buffer done
			
			//create carrier
			if (currentVocoderState->carrier == NOISE) {
				//do noise
				filteredValue = filteredValue * filteredNoiseValue * 2*(currentAmountOfBandpasses+1); //we lose quite a bit of volume, when using noise so amplify it
			}
			else if (currentVocoderState->carrier==SINE) {
				filteredValue = filteredValue * SINEWAVEVECTOR[sinepointer[filterband]];
				sinepointer[filterband] = (int)(sinepointer[filterband] + CENTERFREQUENCIES[currentAmountOfBandpasses - 1][filterband]) % 44100;
			}
			else if (currentVocoderState->carrier == SQUARE) {
				filteredValue = filteredValue * SQUAREWAVEVECTOR[sinepointer[filterband]];
				sinepointer[filterband] = (int)(sinepointer[filterband] + CENTERFREQUENCIES[currentAmountOfBandpasses - 1][filterband]) % 44100;
			}
			else if (currentVocoderState->carrier == SAW) {
				filteredValue = filteredValue * SAWWAVEVECTOR[sinepointer[filterband]];
				sinepointer[filterband] = (int)(sinepointer[filterband] + CENTERFREQUENCIES[currentAmountOfBandpasses - 1][filterband]) % 44100;
			}
			else {
				//OTHERCARRIERS
			}

			if (currentVocoderState->postProcessing == POSTFILTER) {
				//push in the filteredValue, again we should actually do this everytime but for perfomance we only do it if we actually use it
				l2f = _mm256_permute_pd(previousPostFilterInputValues_1[filterband], 0x5);
				lo = _mm256_extractf128_pd(l2f, 0);
				lolo = _mm256_set_m128d(lo, lo);
				blend = _mm256_blend_pd(l2f, lolo, 0x4);
				l2f = _mm256_permute_pd(previousPostFilterInputValues_0[filterband], 0x5);
				hi = _mm256_extractf128_pd(l2f, 1);
				hihi = _mm256_set_m128d(hi, hi);
				previousPostFilterInputValues_1[filterband] = _mm256_blend_pd(blend, hihi, 0x1);
				//first buffer done
				lo = _mm256_extractf128_pd(l2f, 0);
				lolo = _mm256_set_m128d(lo, lo);
				blend = _mm256_blend_pd(l2f, lolo, 0x5);
				in = _mm256_set1_pd(filteredValue);
				previousPostFilterInputValues_0[filterband] = _mm256_blend_pd(blend, in, 0x1);

				//postfilteredValue = 0;
				//POSTFILTER HERE
				
				postfilteredValue = hsum256_pd(
					_mm256_sub_pd(
						_mm256_add_pd(
							_mm256_mul_pd(ALLBANDPASSFILTERNUMERATORCOEFFICIENTS256_0[currentAmountOfBandpasses - 1][filterband], previousPostFilterInputValues_0[filterband]),
							_mm256_mul_pd(ALLBANDPASSFILTERNUMERATORCOEFFICIENTS256_1[currentAmountOfBandpasses - 1][filterband], previousPostFilterInputValues_1[filterband])
						),
						_mm256_add_pd(
							_mm256_mul_pd(ALLBANDPASSFILTERDENOMINATORCOEFFICIENTS256_0[currentAmountOfBandpasses - 1][filterband], previousPostFilterOutputValues_0[filterband]),
							_mm256_mul_pd(ALLBANDPASSFILTERDENOMINATORCOEFFICIENTS256_1[currentAmountOfBandpasses - 1][filterband], previousPostFilterOutputValues_1[filterband])
						)
					)
				);
				
				//push back the filteredvalue
				l2f = _mm256_permute_pd(previousPostFilterOutputValues_1[filterband], 0x5);
				lo = _mm256_extractf128_pd(l2f, 0);
				lolo = _mm256_set_m128d(lo, lo);
				blend = _mm256_blend_pd(l2f, lolo, 0x4);
				l2f = _mm256_permute_pd(previousPostFilterOutputValues_0[filterband], 0x5);
				hi = _mm256_extractf128_pd(l2f, 1);
				hihi = _mm256_set_m128d(hi, hi);
				previousPostFilterOutputValues_1[filterband] = _mm256_blend_pd(blend, hihi, 0x1);
				//first buffer done
				lo = _mm256_extractf128_pd(l2f, 0);
				lolo = _mm256_set_m128d(lo, lo);
				blend = _mm256_blend_pd(l2f, lolo, 0x5);
				in = _mm256_set1_pd(postfilteredValue);
				previousPostFilterOutputValues_0[filterband] = _mm256_blend_pd(blend, in, 0x1);
				SignalBufferOut[i] += postfilteredValue*FILTERWEIGHTS[currentAmountOfBandpasses-1][filterband]; //use postfiltervalue
			}
			else {
				SignalBufferOut[i] += filteredValue * FILTERWEIGHTS[currentAmountOfBandpasses - 1][filterband]; //use unpostfiltered Value
			}

			/*
			NOTE ON PERFORMANCE:
			if we only filter and perform pushbacks on whats actually used
			without Postfiltering:
				Sine: 20 bands
				Noise: ~18/19 bandds
			with Postfiltering:
				Sine: ~14 bands
				Noise: ~13 bands
			*/

		}//all filterbands done
	}//all values done
}

//this function does the whole vocoding
void vocode_accurate() {

	//for each inputvalue
	for (int i = 0; i < BUFFERSIZE; i++) {

		//push in the incoming value
		l2f = _mm256_permute_pd(previousInputValues256_1, 0x5);
		lo = _mm256_extractf128_pd(l2f, 0);
		lolo = _mm256_set_m128d(lo, lo);
		blend = _mm256_blend_pd(l2f, lolo, 0x4);
		l2f = _mm256_permute_pd(previousInputValues256_0, 0x5);
		hi = _mm256_extractf128_pd(l2f, 1);
		hihi = _mm256_set_m128d(hi, hi);
		previousInputValues256_1 = _mm256_blend_pd(blend, hihi, 0x1);
		//first buffer done
		lo = _mm256_extractf128_pd(l2f, 0);
		lolo = _mm256_set_m128d(lo, lo);
		blend = _mm256_blend_pd(l2f, lolo, 0x5);
		in = _mm256_set1_pd((double)SignalBufferIn[i]);
		previousInputValues256_0 = _mm256_blend_pd(blend, in, 0x1);
		//second buffer done


		//push in the noise
		l2f = _mm256_permute_pd(previousNoiseInputValues256_1, 0x5);
		lo = _mm256_extractf128_pd(l2f, 0);
		lolo = _mm256_set_m128d(lo, lo);
		blend = _mm256_blend_pd(l2f, lolo, 0x4);
		l2f = _mm256_permute_pd(previousNoiseInputValues256_0, 0x5);
		hi = _mm256_extractf128_pd(l2f, 1);
		hihi = _mm256_set_m128d(hi, hi);
		previousNoiseInputValues256_1 = _mm256_blend_pd(blend, hihi, 0x1);
		//first buffer done
		lo = _mm256_extractf128_pd(l2f, 0);
		lolo = _mm256_set_m128d(lo, lo);
		blend = _mm256_blend_pd(l2f, lolo, 0x5);
		if (currentVocoderState->carrier == NOISE) {
			in = _mm256_set1_pd(NOISEVECTOR[noisepointer]);
		}
		else {
			in = _mm256_setzero_pd();
		}
			
		noisepointer++; noisepointer %= 44100;
		previousNoiseInputValues256_0 = _mm256_blend_pd(blend, in, 0x1);
		//second buffer done

		//zero Outvalue
		SignalBufferOut[i] = 0;

		//filter the value for each band
		for (int filterband = 0; filterband < currentAmountOfBandpasses; filterband++) {
			filteredValue = 0;
			filteredNoiseValue = 0;
			//mult add
			filteredValue = hsum256_pd(
				_mm256_sub_pd(
					_mm256_add_pd(
						_mm256_mul_pd(ALLBANDPASSFILTERNUMERATORCOEFFICIENTS256_0[currentAmountOfBandpasses - 1][filterband], previousInputValues256_0),
						_mm256_mul_pd(ALLBANDPASSFILTERNUMERATORCOEFFICIENTS256_1[currentAmountOfBandpasses - 1][filterband], previousInputValues256_1)
					),
					_mm256_add_pd(
						_mm256_mul_pd(ALLBANDPASSFILTERDENOMINATORCOEFFICIENTS256_0[currentAmountOfBandpasses - 1][filterband], previousBandpassOutputValues256_0[filterband]),
						_mm256_mul_pd(ALLBANDPASSFILTERDENOMINATORCOEFFICIENTS256_1[currentAmountOfBandpasses - 1][filterband], previousBandpassOutputValues256_1[filterband])
					)
				)
			);
			//filter Noise
				filteredNoiseValue = hsum256_pd(
					_mm256_sub_pd(
						_mm256_add_pd(
							_mm256_mul_pd(ALLBANDPASSFILTERNUMERATORCOEFFICIENTS256_0[currentAmountOfBandpasses - 1][filterband], previousNoiseInputValues256_0),
							_mm256_mul_pd(ALLBANDPASSFILTERNUMERATORCOEFFICIENTS256_1[currentAmountOfBandpasses - 1][filterband], previousNoiseInputValues256_1)
						),
						_mm256_add_pd(
							_mm256_mul_pd(ALLBANDPASSFILTERDENOMINATORCOEFFICIENTS256_0[currentAmountOfBandpasses - 1][filterband], previousBandpassNoiseOutputValues256_0[filterband]),
							_mm256_mul_pd(ALLBANDPASSFILTERDENOMINATORCOEFFICIENTS256_1[currentAmountOfBandpasses - 1][filterband], previousBandpassNoiseOutputValues256_1[filterband])
						)
					)
				);
			


			//check if we need to divide you can uncomment this if you need it, but usually the first denominator is 1
			/*
			if (DIVISIONBYDENOMINATOR0) {
				filteredValue = filteredValue / DENOMINATOR0COEFFICIENT[currentAmountOfBandpasses - 1][filterband];
				filteredNoiseValue = filteredNoiseValue / DENOMINATOR0COEFFICIENT[currentAmountOfBandpasses - 1][filterband];
			}
			*/
			/*
			if (isnan(filteredValue)) {
				std::cout << "bandpassfilter: " << filterband << " seems to be unstable\n";
			}

			if (isnan(filteredNoiseValue)) {
				std::cout << "bandpassfilter : "<<filterband<<" seems to be unstable for noise\n";
			}
			*/


			//pushback the filtered value
			l2f = _mm256_permute_pd(previousBandpassOutputValues256_1[filterband], 0x5);
			lo = _mm256_extractf128_pd(l2f, 0);
			lolo = _mm256_set_m128d(lo, lo);
			blend = _mm256_blend_pd(l2f, lolo, 0x4);
			l2f = _mm256_permute_pd(previousBandpassOutputValues256_0[filterband], 0x5);
			hi = _mm256_extractf128_pd(l2f, 1);
			hihi = _mm256_set_m128d(hi, hi);
			previousBandpassOutputValues256_1[filterband] = _mm256_blend_pd(blend, hihi, 0x1);
			//first buffer done
			lo = _mm256_extractf128_pd(l2f, 0);
			lolo = _mm256_set_m128d(lo, lo);
			blend = _mm256_blend_pd(l2f, lolo, 0x5);
			in = _mm256_set1_pd((double)filteredValue);
			previousBandpassOutputValues256_0[filterband] = _mm256_blend_pd(blend, in, 0x1);
			//second buffer done


			//push back the filtered Noise
			//pushback the filtered value
			l2f = _mm256_permute_pd(previousBandpassNoiseOutputValues256_1[filterband], 0x5);
			lo = _mm256_extractf128_pd(l2f, 0);
			lolo = _mm256_set_m128d(lo, lo);
			blend = _mm256_blend_pd(l2f, lolo, 0x4);
			l2f = _mm256_permute_pd(previousBandpassNoiseOutputValues256_0[filterband], 0x5);
			hi = _mm256_extractf128_pd(l2f, 1);
			hihi = _mm256_set_m128d(hi, hi);
			previousBandpassNoiseOutputValues256_1[filterband] = _mm256_blend_pd(blend, hihi, 0x1);
			//first buffer done
			lo = _mm256_extractf128_pd(l2f, 0);
			lolo = _mm256_set_m128d(lo, lo);
			blend = _mm256_blend_pd(l2f, lolo, 0x5);
			in = _mm256_set1_pd(filteredNoiseValue);
			previousBandpassNoiseOutputValues256_0[filterband] = _mm256_blend_pd(blend, in, 0x1);
			


			//lowpassfilter the value
			
			//we use the absolut outputvalue of the bandpass, so no need to perform a pushback
			filteredValue = hsum256_pd(
				_mm256_sub_pd(
					_mm256_add_pd(
						_mm256_mul_pd(LOWPASSFILTERNUMERATORCOEFFICIENTS256_0,
							_mm256_sqrt_pd(_mm256_mul_pd(previousBandpassOutputValues256_0[filterband], previousBandpassOutputValues256_0[filterband]))
						),
						_mm256_mul_pd(LOWPASSFILTERNUMERATORCOEFFICIENTS256_1,
							_mm256_sqrt_pd(_mm256_mul_pd(previousBandpassOutputValues256_1[filterband], previousBandpassOutputValues256_1[filterband]))
						)
					),
					_mm256_add_pd(
						_mm256_mul_pd(LOWPASSFILTERDENOMINATORCOEFFICIENTS256_0, previousLowpassFilterOutputValues256_0[filterband]),
						_mm256_mul_pd(LOWPASSFILTERDENOMINATORCOEFFICIENTS256_1, previousLowpassFilterOutputValues256_1[filterband])
					)
				)
			);

			//divide if needed, uncomment this if you need it
			/*
			if (LOWPASSDENOMINATOR0 != 1) {
				filteredValue = filteredValue / LOWPASSDENOMINATOR0;
			}
			*/
			/*
			if (isnan(filteredValue)) {
				std::cout << "lowpassfilter seems to be unstable\n";
			}
			*/

			//push back the outputvalue
			l2f = _mm256_permute_pd(previousLowpassFilterOutputValues256_1[filterband], 0x5);
			lo = _mm256_extractf128_pd(l2f, 0);
			lolo = _mm256_set_m128d(lo, lo);
			blend = _mm256_blend_pd(l2f, lolo, 0x4);
			l2f = _mm256_permute_pd(previousLowpassFilterOutputValues256_0[filterband], 0x5);
			hi = _mm256_extractf128_pd(l2f, 1);
			hihi = _mm256_set_m128d(hi, hi);
			previousLowpassFilterOutputValues256_1[filterband] = _mm256_blend_pd(blend, hihi, 0x1);
			//first buffer done
			lo = _mm256_extractf128_pd(l2f, 0);
			lolo = _mm256_set_m128d(lo, lo);
			blend = _mm256_blend_pd(l2f, lolo, 0x5);
			in = _mm256_set1_pd((double)filteredValue);
			previousLowpassFilterOutputValues256_0[filterband] = _mm256_blend_pd(blend, in, 0x1);
			//second buffer done

			//create carrier
			if (currentVocoderState->carrier == NOISE) {
				//do noise
				filteredValue = filteredValue * filteredNoiseValue * 2 * (currentAmountOfBandpasses + 1);
			}
			else if (currentVocoderState->carrier == SINE) {
				filteredValue = filteredValue * SINEWAVEVECTOR[sinepointer[filterband]];
				sinepointer[filterband] = (int)(sinepointer[filterband] + CENTERFREQUENCIES[currentAmountOfBandpasses - 1][filterband]) % 44100;
			}
			else if (currentVocoderState->carrier == SQUARE) {
				filteredValue = filteredValue * SQUAREWAVEVECTOR[sinepointer[filterband]];
				sinepointer[filterband] = (int)(sinepointer[filterband] + CENTERFREQUENCIES[currentAmountOfBandpasses - 1][filterband]) % 44100;
			}
			else if (currentVocoderState->carrier == SAW) {
				filteredValue = filteredValue * SAWWAVEVECTOR[sinepointer[filterband]];
				sinepointer[filterband] = (int)(sinepointer[filterband] + CENTERFREQUENCIES[currentAmountOfBandpasses - 1][filterband]) % 44100;
			}
			else {
				//OTHERCARRIERS
			}

			
			//push in the filteredValue
			l2f = _mm256_permute_pd(previousPostFilterInputValues_1[filterband], 0x5);
			lo = _mm256_extractf128_pd(l2f, 0);
			lolo = _mm256_set_m128d(lo, lo);
			blend = _mm256_blend_pd(l2f, lolo, 0x4);
			l2f = _mm256_permute_pd(previousPostFilterInputValues_0[filterband], 0x5);
			hi = _mm256_extractf128_pd(l2f, 1);
			hihi = _mm256_set_m128d(hi, hi);
			previousPostFilterInputValues_1[filterband] = _mm256_blend_pd(blend, hihi, 0x1);
			//first buffer done
			lo = _mm256_extractf128_pd(l2f, 0);
			lolo = _mm256_set_m128d(lo, lo);
			blend = _mm256_blend_pd(l2f, lolo, 0x5);
			in = _mm256_set1_pd(filteredValue);
			previousPostFilterInputValues_0[filterband] = _mm256_blend_pd(blend, in, 0x1);

				//postfilteredValue = 0;
				//POSTFILTER HERE
			if (currentVocoderState->postProcessing == POSTFILTER) {
				postfilteredValue = hsum256_pd(
					_mm256_sub_pd(
						_mm256_add_pd(
							_mm256_mul_pd(ALLBANDPASSFILTERNUMERATORCOEFFICIENTS256_0[currentAmountOfBandpasses - 1][filterband], previousPostFilterInputValues_0[filterband]),
							_mm256_mul_pd(ALLBANDPASSFILTERNUMERATORCOEFFICIENTS256_1[currentAmountOfBandpasses - 1][filterband], previousPostFilterInputValues_1[filterband])
						),
						_mm256_add_pd(
							_mm256_mul_pd(ALLBANDPASSFILTERDENOMINATORCOEFFICIENTS256_0[currentAmountOfBandpasses - 1][filterband], previousPostFilterOutputValues_0[filterband]),
							_mm256_mul_pd(ALLBANDPASSFILTERDENOMINATORCOEFFICIENTS256_1[currentAmountOfBandpasses - 1][filterband], previousPostFilterOutputValues_1[filterband])
						)
					)
				);
			}
			//push back the postfilteredvalue
			l2f = _mm256_permute_pd(previousPostFilterOutputValues_1[filterband], 0x5);
			lo = _mm256_extractf128_pd(l2f, 0);
			lolo = _mm256_set_m128d(lo, lo);
			blend = _mm256_blend_pd(l2f, lolo, 0x4);
			l2f = _mm256_permute_pd(previousPostFilterOutputValues_0[filterband], 0x5);
			hi = _mm256_extractf128_pd(l2f, 1);
			hihi = _mm256_set_m128d(hi, hi);
			previousPostFilterOutputValues_1[filterband] = _mm256_blend_pd(blend, hihi, 0x1);
			//first buffer done
			lo = _mm256_extractf128_pd(l2f, 0);
			lolo = _mm256_set_m128d(lo, lo);
			blend = _mm256_blend_pd(l2f, lolo, 0x5);
			in = _mm256_set1_pd(postfilteredValue);
			previousPostFilterOutputValues_0[filterband] = _mm256_blend_pd(blend, in, 0x1);
			if(currentVocoderState->postProcessing==POSTFILTER){
				SignalBufferOut[i] += postfilteredValue * FILTERWEIGHTS[currentAmountOfBandpasses - 1][filterband]; //use postfiltervalue
			}
			else {
				SignalBufferOut[i] += filteredValue * FILTERWEIGHTS[currentAmountOfBandpasses - 1][filterband]; //use unpostfiltered Value
			}
			/*
			NOTE ON PERFORMANCE: this is an accurate version, where we push in zero-values even if we dont use them
			without Postfiltering:
				Sine: ~15 bands
				Noise:	~14 bands
			with Postfiltering:
				Sine: ~14 bands
				Noise: ~13 bands
			*/

		}//all filterbands done
	}//all values done
}





//performs some testruns to see if the processing is fast enough
void performTest() {
	//perform a check if we are fast enough
	currentAmountOfBandpasses = MAXNUMOFBANDPASSBANDS; //test with maximum
	//set other parameters if you want to like carriers postfilter etc.
	double avg = 0.0;
	int maxruns = 3;
	for (int i = 0; i < maxruns; i++) {
		auto start = std::chrono::high_resolution_clock::now();
		vocode();
		auto finish = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> elapsed = finish - start;
		std::cout << "Run " << i << "took: " << elapsed.count() << "\n";
		avg += elapsed.count() / maxruns;
	}
	std::cout << "We needed " << avg << " on average in " << maxruns << " runs ";
	double goal = 1.0 / (44100.0 / BUFFERSIZE);
	std::cout << "it needs to be: " << goal << "\n";
	currentAmountOfBandpasses = 1; //reset to one band
}


//sets up the structures and reads all the necessary files
void setupForVocoding(int buffSize) {
	BUFFERSIZE = buffSize;
	/*
	CARRIERS
	*/
	createSineWave();
	createNoise();
	createSquare();
	createSaw();
	/*
	INPUTVALUES
	*/
	previousInputValues256_0 = _mm256_setzero_pd();
	previousInputValues256_1 = _mm256_setzero_pd();
	previousNoiseInputValues256_0 = _mm256_setzero_pd();
	previousNoiseInputValues256_1 = _mm256_setzero_pd();

	/*
	BANDPASSCOEFFICIENTS
	*/
	MAXNUMOFCOEFFICIENTS = 8; //we dont use more than 8 coeffs
	NUMOF256BUFFERSFORCOEFFICIENTS = MAXNUMOFCOEFFICIENTS / 4; //4 values per buffer
	readFilterFromFile(&MAXNUMOFBANDPASSBANDS, &NUMOFNUMERATORCOEFFICIENTSPERFILTER, &NUMOFDENOMINATORCOEFFICIENTSPERFILTER, &ALLFILTERNUMERATORCOEFFICIENTS, &ALLFILTERDENOMINATORCOEFFICIENTS, MAXNUMOFCOEFFICIENTS, &DENOMINATOR0COEFFICIENT);
	//printCoefficientsPerFilter();
	//printAllFilterCoefficients();

	//load coefficients into 256d buffers
	ALLBANDPASSFILTERNUMERATORCOEFFICIENTS256_0.resize(MAXNUMOFBANDPASSBANDS);
	ALLBANDPASSFILTERNUMERATORCOEFFICIENTS256_1.resize(MAXNUMOFBANDPASSBANDS);
	for (int i = 0; i < MAXNUMOFBANDPASSBANDS; i++) {
		ALLBANDPASSFILTERNUMERATORCOEFFICIENTS256_0[i].resize(MAXNUMOFBANDPASSBANDS);
		ALLBANDPASSFILTERNUMERATORCOEFFICIENTS256_1[i].resize(MAXNUMOFBANDPASSBANDS);
		for (int j = 0; j < MAXNUMOFBANDPASSBANDS; j++) {
			ALLBANDPASSFILTERNUMERATORCOEFFICIENTS256_0[i][j] = _mm256_load_pd(&ALLFILTERNUMERATORCOEFFICIENTS[i][j][0]);
			ALLBANDPASSFILTERNUMERATORCOEFFICIENTS256_1[i][j] = _mm256_load_pd(&ALLFILTERNUMERATORCOEFFICIENTS[i][j][4]);
		}
	}
	ALLBANDPASSFILTERDENOMINATORCOEFFICIENTS256_0.resize(MAXNUMOFBANDPASSBANDS);
	ALLBANDPASSFILTERDENOMINATORCOEFFICIENTS256_1.resize(MAXNUMOFBANDPASSBANDS);
	for (int i = 0; i < MAXNUMOFBANDPASSBANDS; i++) {
		ALLBANDPASSFILTERDENOMINATORCOEFFICIENTS256_0[i].resize(MAXNUMOFBANDPASSBANDS);
		ALLBANDPASSFILTERDENOMINATORCOEFFICIENTS256_1[i].resize(MAXNUMOFBANDPASSBANDS);
		for (int j = 0; j < MAXNUMOFBANDPASSBANDS; j++) {
			ALLBANDPASSFILTERDENOMINATORCOEFFICIENTS256_0[i][j] = _mm256_load_pd(&ALLFILTERDENOMINATORCOEFFICIENTS[i][j][0]);
			ALLBANDPASSFILTERDENOMINATORCOEFFICIENTS256_1[i][j] = _mm256_load_pd(&ALLFILTERDENOMINATORCOEFFICIENTS[i][j][4]);
		}
	}//loaded numerators and denominators into 256 buffers

	//check if we need to do a division at the end of the passbands
	for (int i = 0; i < MAXNUMOFBANDPASSBANDS; i++) {
		for (int j = 0; j < MAXNUMOFBANDPASSBANDS; j++) {
			if (j <= i) {
				if (DENOMINATOR0COEFFICIENT[i][j] != 1) {
					DIVISIONBYDENOMINATOR0 = true;
				}
			}
		}
	}

	previousBandpassOutputValues256_0.resize(MAXNUMOFBANDPASSBANDS);
	previousBandpassOutputValues256_1.resize(MAXNUMOFBANDPASSBANDS);
	for (int i = 0; i < MAXNUMOFBANDPASSBANDS; i++) {
		previousBandpassOutputValues256_0[i] = _mm256_setzero_pd();
		previousBandpassOutputValues256_1[i] = _mm256_setzero_pd();
	}

	previousBandpassNoiseOutputValues256_0.resize(MAXNUMOFBANDPASSBANDS);
	previousBandpassNoiseOutputValues256_1.resize(MAXNUMOFBANDPASSBANDS);
	for (int i = 0; i < MAXNUMOFBANDPASSBANDS; i++) {
		previousBandpassNoiseOutputValues256_0[i] = _mm256_setzero_pd();
		previousBandpassNoiseOutputValues256_1[i] = _mm256_setzero_pd();
	}



	//read the lowpassfilter
	MAXNUMOFLOWPASSCOEFFICIENTS = 8;
	NUMOF256BUFFERFORLOWPASSCOEFFICIENTS = MAXNUMOFLOWPASSCOEFFICIENTS / 4;
	ReadLowPassForEnvelope(MAXNUMOFLOWPASSCOEFFICIENTS, NUMOF256BUFFERFORLOWPASSCOEFFICIENTS, &NUMOFLOWPASSNUMERATORCOEFFICIENTS, &NUMOFLOWPASSDENOMINATORCOEFFICIENTS, &LOWPASSFILTERNUMERATORCOEFFICIENTS, &LOWPASSFILTERDENOMINATORCOEFFICIENTS, &LOWPASSDENOMINATOR0);

	LOWPASSFILTERNUMERATORCOEFFICIENTS256_0 = _mm256_load_pd(&LOWPASSFILTERNUMERATORCOEFFICIENTS[0]);
	LOWPASSFILTERNUMERATORCOEFFICIENTS256_1 = _mm256_load_pd(&LOWPASSFILTERNUMERATORCOEFFICIENTS[4]);
	LOWPASSFILTERDENOMINATORCOEFFICIENTS256_0 = _mm256_load_pd(&LOWPASSFILTERDENOMINATORCOEFFICIENTS[0]);
	LOWPASSFILTERDENOMINATORCOEFFICIENTS256_1 = _mm256_load_pd(&LOWPASSFILTERDENOMINATORCOEFFICIENTS[4]);

	previousLowpassFilterInputValues256_0.resize(MAXNUMOFBANDPASSBANDS);
	previousLowpassFilterInputValues256_1.resize(MAXNUMOFBANDPASSBANDS);
	for (int i = 0; i < MAXNUMOFBANDPASSBANDS; i++) {
		previousLowpassFilterInputValues256_0[i] = _mm256_setzero_pd();
		previousLowpassFilterInputValues256_1[i] = _mm256_setzero_pd();
	}


	previousLowpassFilterOutputValues256_0.resize(MAXNUMOFBANDPASSBANDS);
	previousLowpassFilterOutputValues256_1.resize(MAXNUMOFBANDPASSBANDS);
	for (int i = 0; i < MAXNUMOFBANDPASSBANDS; i++) {
		previousLowpassFilterOutputValues256_0[i] = _mm256_setzero_pd();
		previousLowpassFilterOutputValues256_1[i] = _mm256_setzero_pd();
	}

	//postfilter
	previousPostFilterInputValues_0.resize(MAXNUMOFBANDPASSBANDS);
	previousPostFilterInputValues_1.resize(MAXNUMOFBANDPASSBANDS);
	for (int i = 0; i < MAXNUMOFBANDPASSBANDS; i++) {
		previousPostFilterInputValues_0[i] = _mm256_setzero_pd();
		previousPostFilterInputValues_1[i] = _mm256_setzero_pd();
	}

	previousPostFilterOutputValues_0.resize(MAXNUMOFBANDPASSBANDS);
	previousPostFilterOutputValues_1.resize(MAXNUMOFBANDPASSBANDS);
	for (int i = 0; i < MAXNUMOFBANDPASSBANDS; i++) {
		previousPostFilterOutputValues_0[i] = _mm256_setzero_pd();
		previousPostFilterOutputValues_1[i] = _mm256_setzero_pd();
	}


	//read the frequencies
	ReadCenterFrequencies(MAXNUMOFBANDPASSBANDS, &CENTERFREQUENCIES);
	//printCenterFrequecies();
	ReadFilterWeights(MAXNUMOFBANDPASSBANDS, &FILTERWEIGHTS);
	//init the pointers
	sinepointer.resize(MAXNUMOFBANDPASSBANDS);
	for (int i = 0; i < MAXNUMOFBANDPASSBANDS; i++) {
		sinepointer[i] = 0;
	}
	noisepointer = 0;

	//assign vocode function
	//vocode = &vocode_fast; //fast version
	vocode = &vocode_accurate; //more accurate but slower
	//preformTest(); //check if we are fast enough
}	

//currently not used
void resetBufferValues() {
	previousInputValues256_0 = _mm256_setzero_pd();
	previousInputValues256_1 = _mm256_setzero_pd();
	previousNoiseInputValues256_0 = _mm256_setzero_pd();
	previousNoiseInputValues256_1 = _mm256_setzero_pd();
	for (int i = 0; i < MAXNUMOFBANDPASSBANDS; i++) {
		//reset bandpass outs
		previousBandpassOutputValues256_0[i] = _mm256_setzero_pd();
		previousBandpassOutputValues256_1[i] = _mm256_setzero_pd();
		previousBandpassNoiseOutputValues256_0[i] = _mm256_setzero_pd();
		previousBandpassNoiseOutputValues256_1[i] = _mm256_setzero_pd();
		//reset lowpass outs
		previousLowpassFilterOutputValues256_0[i] = _mm256_setzero_pd();
		previousLowpassFilterOutputValues256_1[i] = _mm256_setzero_pd();
		//reset postfilter
		previousPostFilterInputValues_0[i] = _mm256_setzero_pd();
		previousPostFilterInputValues_1[i] = _mm256_setzero_pd();
		previousPostFilterOutputValues_0[i] = _mm256_setzero_pd();
		previousPostFilterOutputValues_1[i] = _mm256_setzero_pd();
	}
};