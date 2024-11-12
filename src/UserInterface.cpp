#include "../include/UserInterface.h"
//#include <limits>
//#include <cctype>

void UserInterface::showCalibrationInfo(const Histogram &histogram) const
{
    std::cout << "   matched peaks: " << histogram.getPeakMatchCount() << std::endl;
}

double *UserInterface::askAboutSource(CalibrationDataProvider &energys, int &size, std::string &sourceName, int &numberOfPeaks)
{
    std::vector<double *> selectedEnergyArrays;
    std::vector<int> arraySizes;
    bool addMoreSources = true;
    int totalSize = 0;

    while (addMoreSources)
    {
        // Display available sources
        std::cout << "Available sources:" << std::endl;
        energys.printSources();

        // Ask user for the desired source
        std::cout << "Which source do you want? (Enter the source number)" << std::endl;
        int sourceNumber;
        std::cin >> sourceNumber;

        // Validate source number
        if (std::cin.fail() || sourceNumber < 0 || sourceNumber >= energys.getSize())
        {
            std::cin.clear();                                                   // clear input stream
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // discard invalid input
            std::cerr << "Invalid source number!" << std::endl;
            continue;
        }
        double *energyArray = energys.getCalibratedEnergyArray(sourceNumber);
        int arraySize = energys.getCalibratedEnergyArraySize(sourceNumber);
        numberOfPeaks += energys.getNumberOfPeaks(sourceNumber);
        sourceName += energys.getSourceName(sourceNumber);
        std::cout << "Source name: " << energys.getSourceName(sourceNumber) << std::endl;
        std::cout << "Array size: " << arraySize << std::endl;

        if (energyArray)
        {
            selectedEnergyArrays.push_back(energyArray);
            arraySizes.push_back(arraySize);
            totalSize += arraySize;
            std::cout << "Total size so far: " << totalSize << std::endl;
        }

        // Ask if more sources are to be added
        std::cout << "Do you want to add more sources? (Y/N)" << std::endl;
        char answer;
        std::cin >> answer;
        addMoreSources = (tolower(answer) == 'y');
    }

    // Combine all selected energy arrays into one
    if (totalSize == 0) // Handle the case where no sources are selected
    {
        std::cerr << "No sources selected!" << std::endl;
        size = 0;
        return nullptr;
    }

    double *combinedEnergyArray = new double[totalSize];
    int index = 0;
    for (size_t i = 0; i < selectedEnergyArrays.size(); ++i)
    {
        int arraySize = arraySizes[i];
        std::copy(selectedEnergyArrays[i], selectedEnergyArrays[i] + arraySize, combinedEnergyArray + index);
        index += arraySize;
    }

    size = totalSize; // Set the total size for the caller
    return combinedEnergyArray;
}

void UserInterface::askAboutPeaks(std::vector<Histogram> &histograms, std::ofstream &jsonFile, TFile *outputFileHistograms, TFile *outputFileCalibrated)
{
    std::cout << "Do you want to change a peak? (Y/N)" << std::endl;
    char answer;
    std::cin >> answer;

    if (tolower(answer) != 'y')
        return;

    std::cout << "Available histograms:" << std::endl;
    for (size_t i = 0; i < histograms.size(); ++i)
    {
        const char *histName = histograms[i].returnNameOfHistogram();
        auto mainHist = histograms[i].getMainHist();
        if (histName && std::strlen(histName) > 0 && mainHist && mainHist->GetMean() > 5)
        {
            std::cout << i << ". " << histName << std::endl;
        }
    }

    while (tolower(answer) == 'y')
    {
        std::cout << "In which histogram is the peak?" << std::endl;
        size_t histogramNumber;
        std::cin >> histogramNumber;

        if (histogramNumber >= histograms.size())
        {
            std::cerr << "Invalid histogram number!" << std::endl;
            continue;
        }

        std::cout << "What is the number of the peak?" << std::endl;
        int peakNumber;
        std::cin >> peakNumber;

        std::cout << "What is the new position of the peak?" << std::endl;
        double newPosition;
        std::cin >> newPosition;

        histograms[histogramNumber].changePeak(peakNumber, newPosition);
        histograms[histogramNumber].outputPeaksDataJson(jsonFile);
        histograms[histogramNumber].printHistogramWithPeaksRoot(outputFileHistograms);

        std::cout << "Do you want to change another peak? (Y/N)" << std::endl;
        std::cin >> answer;
    }
}