#ifndef HISTOGRAM_H
#define HISTOGRAM_H

#include "Peak.h"
#include <TH1D.h>
#include <TF1.h>
#include <vector>
#include <iostream>
#include <cmath>
#include <limits>
#include <TFile.h>
#include <TCanvas.h>
#include <TLatex.h>
#include <string>
#include <TGraph.h>
#include <cmath>
//#include <eigen3/Eigen/Dense>
class Histogram
{
private:
    int xMin, xMax;
    int maxFWHM;
    float minAmplitude;
    float maxAmplitude;
    int numberOfPeaks;
    TH1D *mainHist;
    TH1D *tempHist;
    TH1D *calibratedHist;
    std::vector<Peak> peaks;
    int degree;
    float m, b;
    std::vector<double> coefficients;
    int polinomDegree;
    unsigned int peakMatchCount;
    int peakCount;
    std::string TH2histogram_name;
    std::string sourceName;
    std::string serial = "CL";
    int detType = 2;
    float polynomialFitThreshold;
    float totalArea;
    float totalAreaError;
    // Funcții private
    void eliminatePeak(const Peak &peak);
    TF1 *createGaussianFit(int maxBin);
    int findMaxBin();
    int detectAndFitPeaks();
    bool checkPredictedEnergies(double predictedEnergy, const double knownEnergies[], int size, float errorAdmitted, double &valueAssociatedWith) const;
    double refineCalibration(const double knownEnergies[], int size) const;
    double refineCalibrationM();
    double refineCalibrationB();
    void findStartOfPeak(Peak &peak);
    void initializeCalibratedHist();
    double getInterpolatedContent(int bin_original, double binCenter_original) const;
    void getTheDegreeOfPolynomial();
    std::string getMainHistName() const;
    double inversePolynomial(double y) const;
    double evaluatePolynomial(double x) const;

public:
    // Constructori și Destructor
    Histogram();
    Histogram(int xMin, int xMax, int maxFWHM, float minAmplitude, float maxAmplitude, std::string serial, int detType, float polynomialFitThreshold, int numberOfPeaks, TH1D *mainHist, const std::string &TH2histogram_name, std::string sourceName);
    ~Histogram();
    Histogram(const Histogram &histogram);
    Histogram &operator=(const Histogram &histogram);
    // Funcții publice
    void findPeaks();
    bool checkConditions(const Peak &peak) const;
    void outputPeaksDataJson(std::ofstream &file);
    void applyXCalibration();
    void calibratePeaks(const double knownEnergies[], int size);
    void calibratePeaksByDegree();
    void changePeak(int peakNumber, double newPosition);
    void printHistogramWithPeaksRoot(TFile *outputFile);
    void printCalibratedHistogramRoot(TFile *outputFile) const;
    const char *returnNameOfHistogram() const;
    unsigned int getpeakMatchCount() const;
    void setTotalArea(); 
    void setTotalAreaError(); 
    float getPT();
    float getPTError(); 
    TH1D *getCalibratedHist() const;
    TH1D *getMainHist() const;

    void interpolateBins(int start_bin, int end_bin, double start_value, double end_value, double start_position, double end_position);
    void setCalibratedBinContent(int original_bin);
    double evaluateCalibrationPolynomial(double binCenter_original) const;


    bool extractDataForFit(std::vector<double>& xValues, std::vector<double>& yValues);
    double calculateR2(double chi2, int ndf);
    bool areCoefficientsValid(TF1* fitFunction, int degree, double threshold);
    std::vector<double> solveSystem(const std::vector<std::vector<double>>& A, const std::vector<double>& b);
    std::vector<double> multiplyTransposeVector(const std::vector<std::vector<double>>& X, const std::vector<double>& Y);
    std::vector<std::vector<double>> multiplyTransposeMatrix(const std::vector<std::vector<double>>& X);


};

#endif // HISTOGRAM_H
