#include "DataDump.h"

using namespace corryvreckan;

DataDump::DataDump(Configuration config, Clipboard* clipboard) : Algorithm(std::move(config), clipboard) {
    m_detector = "DeviceToDumpData";
}

void DataDump::initialise(Parameters* par) {

    parameters = par;

    // Initialise member variables
    m_eventNumber = 0;
}

StatusCode DataDump::run(Clipboard* clipboard) {

    // Take input directory from global parameters
    string inputDirectory = parameters->inputDirectory + "/" + m_detector;

    // File structure is RunX/ChipID/files.dat

    // Open the root directory
    DIR* directory = opendir(inputDirectory.c_str());
    if(directory == NULL) {
        LOG(ERROR) << "Directory " << inputDirectory << " does not exist";
        return Failure;
    }

    // Get a list of files and store them
    dirent* file;
    vector<string> datafiles;
    int nFiles(0);
    FILE* currentFile = NULL;
    long long int syncTime = 0;
    bool clearedHeader = false;

    // Get all of the files for this chip
    while(file = readdir(directory)) {
        string filename = inputDirectory + "/" + file->d_name;

        // Check if file has extension .dat
        if(string(file->d_name).find("-1.dat") != string::npos) {
            datafiles.push_back(filename.c_str());
            nFiles++;
        }
    }

    // Now loop over the files, and for each file make a dump of the hex values
    // (except for the header)

    for(int fileNumber = 0; fileNumber < 1; fileNumber++) {

        // Open a new file
        currentFile = fopen(datafiles[fileNumber].c_str(), "rb");
        LOG(INFO) << "Loading file " << datafiles[fileNumber];

        // Open a new output file to dump the data to
        ofstream dataDumpFile;
        string dataDumpFilename = "outputHexDump.dat";
        dataDumpFile.open(dataDumpFilename.c_str());

        // Skip the header - first read how big it is
        uint32_t headerID;
        if(fread(&headerID, sizeof(headerID), 1, currentFile) == 0) {
            LOG(ERROR) << "Cannot read header ID";
            return Failure;
        }

        // Skip the rest of the file header
        uint32_t headerSize;
        if(fread(&headerSize, sizeof(headerSize), 1, currentFile) == 0) {
            LOG(ERROR) << "Cannot read header size";
            return Failure;
        }

        // Finally skip the header
        rewind(currentFile);
        fseek(currentFile, headerSize, SEEK_SET);

        // Read till the end of file
        while(!feof(currentFile)) {

            // Read one 64-bit chunk of data
            ULong64_t pixdata = 0;
            const int retval = fread(&pixdata, sizeof(ULong64_t), 1, currentFile);

            // Check if this is a trigger packet for Adrian
            const UChar_t header = ((pixdata & 0xF000000000000000) >> 60) & 0xF;

            // Use header 0x4 to get the long timestamps (called syncTime here)
            if(header == 0x6) {
                const UChar_t header2 = ((pixdata & 0x0F00000000000000) >> 56) & 0xF;
                if(header2 == 0xF) {
                    dataDumpFile << hex << pixdata << dec << endl;
                }
            }
            // dataDumpFile << hex << pixdata << dec << endl;
        }
    }

    // Return value telling analysis to stop
    return Failure;
}

void DataDump::finalise() {

    LOG(DEBUG) << "Analysed " << m_eventNumber << " events";
}
