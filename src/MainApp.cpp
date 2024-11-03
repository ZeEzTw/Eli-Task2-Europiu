#include "../include/Histogram.h"
#include "../include/Peak.h"
#include "../include/UserInterface.h"
#include "../include/ArgumentsManager.h"
#include <iostream>
#include <fstream>
#include <TFile.h>
#include <TH2F.h>
#include <TH1D.h>
#include <TError.h>
#include <sys/stat.h>

std::string removeFileExtension(const std::string &filePath)
{
    size_t lastDot = filePath.find_last_of('.');
    if (lastDot != std::string::npos)
    {
        return filePath.substr(0, lastDot);
    }
    return filePath;
}

std::string extractRunNumber(const std::string &filePath)
{
    size_t pos = 0;
    while ((pos = filePath.find('_', pos)) != std::string::npos)
    {
        size_t start = pos + 1;
        size_t end = start;
        while (end < filePath.size() && std::isdigit(filePath[end]))
        {
            ++end;
        }
        if (end > start)
        {
            return filePath.substr(start, end - start);
        }
        pos = end;
    }
    std::string fileName = removeFileExtension(filePath);
    for (char &ch : fileName)
    {
        if (ch == '/' || ch == '\\')
        {
            ch = '_';
        }
    }
    return fileName;
}

std::string extractDirectoryPath(const std::string &filePath)
{
    size_t lastSlash = filePath.find_last_of("/\\");
    if (lastSlash != std::string::npos)
    {
        return filePath.substr(0, lastSlash + 1);
    }
    return "./"; 
}

void openFiles(const char *inputFilePath, TFile *&inputFile, TFile *&outputFileHistograms, TFile *&outputFileCalibrated, std::ofstream &jsonFile, TFile *&outputFileTH2, const std::string &savePath, const std::string &histogramFilePath)
{
    inputFile = new TFile(inputFilePath, "READ");

    std::string runName = extractRunNumber(inputFilePath);
    std::string baseDirectory = extractDirectoryPath(inputFilePath);
    std::string saveDirectory;

    if (savePath.empty())
    {
        saveDirectory = baseDirectory + runName + "/";
    }
    else
    {
        saveDirectory = savePath;
        if (saveDirectory.back() != '/')
        {
            saveDirectory += '/';
        }
    }
    mkdir(saveDirectory.c_str(), 0777); // POSIX (Unix/Linux)
    std::cout << "Save Directory: " << saveDirectory << std::endl;

    std::string fileName = removeFileExtension(histogramFilePath);
    for (char &ch : fileName)
    {
        if (ch == '/' || ch == '\\')
        {
            ch = '_';
        }
    }

    std::string jsonFilePath = saveDirectory + runName + "_peaks_data.json";
    jsonFile.open(jsonFilePath.c_str());

    outputFileHistograms = new TFile((saveDirectory + runName + "_peaks.root").c_str(), "RECREATE");
    outputFileCalibrated = new TFile((saveDirectory + runName + "_calibrated_histograms.root").c_str(), "RECREATE");
    outputFileTH2 = new TFile((saveDirectory + runName + "_combinedHistogram.root").c_str(), "RECREATE");
}

// Function to close ROOT files and JSON output file
void closeFiles(TFile *inputFile, TFile *outputFileHistograms, TFile *outputFileCalibrated, std::ofstream &jsonFile)
{
    outputFileHistograms->Close();
    outputFileCalibrated->Close();
    inputFile->Close();
    jsonFile.close();

    delete outputFileHistograms;
    delete outputFileCalibrated;
    delete inputFile;
}
// Function to process all columns in a 2D histogram
void convertHistogramsToTH2(const std::vector<Histogram> &histograms, TH2F *inputTH2, TFile *outputFileTH2)
{
    if (histograms.empty() || !inputTH2)
    {
        std::cerr << "5" << std::endl;
        return;
    }

    int number_of_columns = inputTH2->GetNbinsX();
    int number_of_binsY = inputTH2->GetNbinsY();

    inputTH2->Reset();

    int number_of_histograms = histograms.size();
    for (int i = 0; i < number_of_histograms; ++i)
    {
        TH1D *hist1D = histograms[i].getCalibratedHist();
        if (hist1D)
        {
            for (int binY = 1; binY <= hist1D->GetNbinsX(); ++binY)
            {
                double content = hist1D->GetBinContent(binY);
                int xBinIndex = i + 1;

                if (xBinIndex <= number_of_columns && binY <= number_of_binsY)
                {
                    inputTH2->SetBinContent(xBinIndex, binY, content);
                }
            }
        }
    }

    inputTH2->Write();

    if (outputFileTH2)
    {
        outputFileTH2->cd();
        inputTH2->Write();
        outputFileTH2->Close();
        delete outputFileTH2;
    }
}

void processHistogram(ArgumentsManager &arguments, TH1D *hist1D, double *energyArray, int size, std::vector<Histogram> &histograms, std::ofstream &jsonFile, TFile *outputFileHistograms, TFile *outputFileCalibrated, UserInterface &ui)
{
    if (!hist1D || hist1D->GetMean() < 5)
    {
        delete hist1D;
        histograms.emplace_back();
        return;
    }
    Histogram hist;
    int numberOfHistogram = arguments.GetNumberColomSpecified(histograms.size());
    if (arguments.checkIfRunIsValid() && numberOfHistogram != -1)
    {
        std::cout << "Histogram number: " << numberOfHistogram << std::endl;
        hist = Histogram(arguments.getXminFile(numberOfHistogram),
                         arguments.getXmaxFile(numberOfHistogram),
                         arguments.getFWHMmaxFile(numberOfHistogram),
                         arguments.getMinAmplitude(),
                         arguments.getMaxAmplitudeFile(numberOfHistogram),
                         arguments.getSerialFile(numberOfHistogram),
                         arguments.getDetTypeFile(numberOfHistogram),
                         arguments.getPolynomialFitThreshold(),
                         arguments.getNumberOfPeaks(),
                         hist1D,
                         arguments.getHistogramNameFile(numberOfHistogram),
                         arguments.getSourcesName());
    }
    else
    {
        hist = Histogram(arguments.getXmin(),
                         arguments.getXmax(),
                         arguments.getFWHMmax(),
                         arguments.getMinAmplitude(),
                         arguments.getMaxAmplitude(),
                         arguments.getSerialStandard(),
                         arguments.getDetTypeStandard(),
                         arguments.getPolynomialFitThreshold(),
                         arguments.getNumberOfPeaks(),
                         hist1D,
                         arguments.getHistogramName(),
                         arguments.getSourcesName());
    }
    hist.findPeaks();
    hist.calibratePeaks(energyArray, size);
    hist.applyXCalibration();
    hist.outputPeaksDataJson(jsonFile);
    hist.printHistogramWithPeaksRoot(outputFileHistograms);
    hist.printCalibratedHistogramRoot(outputFileCalibrated);
    histograms.push_back(hist);

    if (arguments.isUserInterfaceEnabled())
    {
        std::cout << "Histograms processed: " << histograms.size() - 1 << std::endl;
        ui.showCalibrationInfo(hist);
    }

    delete hist1D;
}

void process2DHistogram(ArgumentsManager &arguments, TH2F *h2, double *energyArray, int size, UserInterface &ui, std::ofstream &jsonFile, TFile *outputFileHistograms, TFile *outputFileCalibrated, TFile *outputFileTH2)
{
    if (!h2)
    {
        std::cerr << "4" << std::endl;
        return;
    }
    std::vector<Histogram> histograms;
    int start_column = 0;
    int number_of_columns = h2->GetNbinsX();
    TH2F *coppiedTh2 = (TH2F *)h2->Clone("coppiedTh2");
    if (arguments.isDomainLimitsSet())
    {
        // std::cout << "Domain limits set" << std::endl;
        number_of_columns = arguments.getXmaxDomain();
        start_column = arguments.getXminDomain();
    }
    else
    {
        // std::cout << "Domain limits not set" << std::endl;//cod de eroare daca e
    }
    for (int column = start_column; column <= number_of_columns; ++column)
    {
        TH1D *hist1D = h2->ProjectionY(Form("hist1D_col%d", column), column, column);
        if (hist1D)
        {
            processHistogram(arguments, hist1D, energyArray, size, histograms, jsonFile, outputFileHistograms, outputFileCalibrated, ui);
        }
    }

    convertHistogramsToTH2(histograms, h2, outputFileTH2);

    if (arguments.isUserInterfaceEnabled())
    {
        ui.askAboutPeaks(histograms, jsonFile, outputFileHistograms, outputFileCalibrated);
    }
}

void processHistogramsTask(ArgumentsManager &arguments)
{
    TFile *inputFile = nullptr;
    TFile *outputFileHistograms = nullptr;
    TFile *outputFileCalibrated = nullptr;
    TFile *outputFileTH2 = nullptr;
    std::ofstream jsonFile;

    openFiles(arguments.getHistogramFilePath().c_str(), inputFile, outputFileHistograms, outputFileCalibrated, jsonFile, outputFileTH2, arguments.getSavePath(), arguments.getHistogramFilePath());
    if (!inputFile)
    {
        std::cerr << "3" << std::endl;
        return;
    }

    TH2F *h2 = nullptr;
    inputFile->GetObject(arguments.getHistogramName().c_str(), h2);

    if (h2)
    {
        double *energyArray = nullptr;
        int size = 0;
        UserInterface ui;

        if (arguments.isUserInterfaceEnabled())
        {
            sortEnergy energyProcessor = arguments.getEnergyProcessor();
            std::string sourcesName;
            int numberOfPeaks = 0;
            energyArray = ui.askAboutSource(energyProcessor, size, sourcesName, numberOfPeaks);
            arguments.setNumberOfPeaks(numberOfPeaks);
            arguments.setSourceName(sourcesName);
            if (!energyArray)
            {
                std::cerr << "Error: Failed to retrieve energy array from UserInterface." << std::endl;
                return;
            }
        }
        else
        {
            energyArray = arguments.getEnergyProcessor().createSourceArray(size);
            arguments.getSourcesNameRun();
            if (!energyArray)
            {
                std::cerr << "2" << std::endl;
            }
        }
        process2DHistogram(arguments, h2, energyArray, size, ui, jsonFile, outputFileHistograms, outputFileCalibrated, outputFileTH2);
        delete[] energyArray;
    }
    else
    {
        std::cerr << "Error: 2D histogram '" << arguments.getHistogramName() << "' not found in the input file." << std::endl;
    }

    closeFiles(inputFile, outputFileHistograms, outputFileCalibrated, jsonFile);
}

int main(int argc, char *argv[])
{
    gErrorIgnoreLevel = kError;

    /*if (argc < 12)
    {
        std::cerr << "Usage: " << argv[0] << " <number_of_peaks> <source_name> <histogram_file_path> <TH2histogram_name> <energy_file_path> <Xmin> <Xmax> <FWHMmax> <MinAmplitude> <MaxAmplitude> <save_path> <sources> " << std::endl;
        std::cerr << "1" << std::endl;
        return 1;
    }

    int number_of_peaks = std::stoi(argv[1]);
    std::string sourceName = argv[2];
    std::string histogramFilePath = argv[3];
    std::string TH2histogram_name = argv[4];
    std::string energyFilePath = argv[5];
    float Xmin = std::stof(argv[6]);
    float Xmax = std::stof(argv[7]);
    float FWHMmax = std::stof(argv[8]);
    float MinAmplitude = std::stof(argv[9]);
    float MaxAmplitude = std::stof(argv[10]);
    std::string savePath = argv[11];
    */
    ArgumentsManager argumentsManager(argc, argv);
    // argumentsManager.printAllArguments();
    // if (argc > 12)
    //{
    argumentsManager.parseJsonFile();
    argumentsManager.printArgumentsInput();
    processHistogramsTask(argumentsManager);
    //}
    // else
    //{
    // processHistogramsTask(number_of_peaks, sourceName, histogramFilePath, TH2histogram_name, energyProcessor, Xmin, Xmax, FWHMmax, MinAmplitude, MaxAmplitude, savePath, histogramFilePath, true);
    //}

    std::cerr << "0" << std::endl;
    return 0;
}
