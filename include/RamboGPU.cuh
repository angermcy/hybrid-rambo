#ifndef _RAMBOGPU_
#define _RAMBOGPU_

#include <vector>
#include <set>
#include <string>
#include <bitset>
#include <utility>

#include "constants.h"
#include "bitArray.h"
#include "Rambo_construction.h"
#include "BloomFilterGPU.cuh"


class RAMBOGPU
{
public:

    int K;
    int R;
    int B;
    int n;
    float p;
    int range;
    float FPR;

    // time in nanoseconds
    float time_copymalloc_ns;
    float time_processing_ns;
    float time_copyback_ns;
    float time_free_ns;

    float time_emptyingBarAndBait;
    float time_findCandidates;
    float time_copyCandidatesToGPU;
    float time_runTests;
    float time_createBar;
    float time_copyResults;

    uint size_metaRambo_by;
    uint size_metaRamboSizesOffsets_by;
    uint size_testPositions_by;
    uint size_testBait_by;
    uint size_barSize_by;


    BloomFilterGPU** Rambo_array;

    uint reducedMRSize;
    uint noOfRMRPerRSize;
    uint mRSizesSize;
    uint barSize;

    std::vector<int>* metaRambo;

    RAMBOGPU(int n, int r1, int b1, int K);

    std::vector<uint> hashfunc( std::string key, int len);

    void insertion (std::string setID, std::vector<std::string> keys);
    void insertion2 (std::vector<std::string> alllines);

    std::vector <std::string> getdata(std::string filenameSet);
    void createMetaRambo(int K, bool verbose);

    bitArray queryGpu(std::string singleQueryKmer, int lengthOfSingleQuery);

    void copyMRtoGPU(void);
    void cleanUpMR(void);

    void serializeRAMBO(std::string dir);
    void deserializeRAMBO(std::vector<std::string> dir);

    void insertion3(std::vector<std::pair<std::string, int>> KeySets);
    void printMetaRambo(void);

    // **** getter for data sizes ****

};

#endif
