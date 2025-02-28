#include <iomanip>
#include <fstream>
#include <iostream>
#include <chrono>
#include <vector>
#include <math.h>
#include <sstream>
#include <string>
#include <string.h>
#include <algorithm>
#include <set>
#include <iterator>
#include <bitset>
#include <iostream>
#include <ratio>
#include <thread>

#include "MurmurHash3.h"
#include "constants.h"
#include "bitArray.h"
#include "utils.h"

#include "BloomFilterGPU.cuh"
#include "RamboGPU.cuh"
#include "deviceFctsGPU.cuh"
#include "bitComputationGPU.cuh"


/*
 * constructor for basic RAMBO structure for GPU
 * constructs pointer array of BloomFilter with given dimension R x B
 */
RAMBOGPU::RAMBOGPU(int n, int r1, int b1, int k)
{
    R = r1;
    B = b1;
    K = k;

    range = n;

    //Rambo_array_save = new BloomFilterGPU *[B * R];
    Rambo_array = new BloomFilterGPU *[B * R];

    metaRambo = new std::vector<int>[B * R];

    for (int b = 0; b < B; b++)
    {
        for (int r = 0; r < R; r++)
        {
            Rambo_array[b + B * r] = new BloomFilterGPU(range, 0.0);
        }
    }
    //aBF = (char *)malloc(B*R*(n/8+1)*sizeof(char));
}

/*
 *  calculates hash values with given string key and given length len via MurmurHash3
 */
std::vector<uint> RAMBOGPU::hashfunc(std::string key, int len)
{
    std::vector<uint> hashvals;
    uint op;
    for (int i = 0; i < R; i++)
    {
        MurmurHash3_x86_32(key.c_str(), len, 10, &op); // seed i
        hashvals.push_back(op % B);
    }
    return hashvals;
}

/*
 *  inserts hash values of vector of strings keys in Rambo_array at position of hash values of string setID
 */
void RAMBOGPU::insertion(std::string setID, std::vector<std::string> keys)
{
    std::vector<uint> hashvals = RAMBOGPU::hashfunc(setID, setID.size());

    // make this loop parallel
    #pragma omp parallel for
    for (std::size_t i = 0; i < keys.size(); ++i)
    {
        for (int r = 0; r < R; r++)
        {
            std::vector<uint> temp = myhash(keys[i].c_str(), keys[i].size(), murmurmaxseed, r, range);
            Rambo_array[hashvals[r] + B * r]->insert(temp);
        }
    }
}

/*
 *  extend of insertion-function by including reading data string setID and vector of strings keys from vector alllines
 */
void RAMBOGPU::insertion2(std::vector<std::string> alllines)
{
    // make this loop parallel
    //#pragma omp parallel for
    for (std::size_t i = 0; i < alllines.size(); ++i)
    {
        char d = ';';
        std::vector<std::string> KeySets = line2array(alllines[i], d); // sets for a key

        std::vector<std::string> KeySet = line2array(KeySets[1], ',');
        for (uint j = 0; j < KeySet.size(); j++)
        {
            std::vector<uint> hashvals = RAMBOGPU::hashfunc(KeySet[j], KeySet[j].size()); // R hashvals
            for (int r = 0; r < R; r++)
            {
                std::vector<uint> temp = myhash(KeySets[0].c_str(), KeySets[0].size(), murmurmaxseed, r, range);
                Rambo_array[hashvals[r] + B * r]->insert(temp);
            }
        }
    }
}

/*
 *  reads out data from a file and saves them in vector allKeys
 */
std::vector<std::string> RAMBOGPU::getdata(std::string filenameSet)
{
    // get the size of Bloom filter by count
    std::ifstream cntfile(filenameSet);
    std::vector<std::string> allKeys;
    int totKmerscnt = 0;
    while (cntfile.good())
    {
        std::string line1, vals;
        while (getline(cntfile, line1))
        {
            std::stringstream is;
            is << line1;
            if (line1[0] != '>' && line1.size() > 30)
            {
                for (uint idx = 0; idx < line1.size() - 31 + 1; idx++)
                {
                    allKeys.push_back(line1.substr(idx, 31));
                    totKmerscnt++;
                }
            }
        }
    }
    std::cout << "total inserted from one file: " << totKmerscnt << std::endl;
    return allKeys;
}

/*
 *  construcs array of dimension R x B called metaRambo to save by hash function generated B groups of documents/queries in it
 *  with optional print out of groups
 */
void RAMBOGPU::createMetaRambo(int K, bool verbose)
{
    //std::vector<int>().swap(metaRambo);
    //metaRambo.clear();

    //metaRambo = new std::vector<int>[B * R];

    for (int i = 0; i < K; i++)
    {
        std::vector<uint> hashvals = RAMBOGPU::hashfunc(std::to_string(i), std::to_string(i).size());
        for (int r = 0; r < R; r++)
        {
            metaRambo[hashvals[r] + B * r].push_back(i);
        }
    }

    // print RAMBOGPU meta deta
    if (verbose)
    {
        for (int b = 0; b < B; b++)
        {
            std::cout << "[" << b << "]";
            for (int r = 0; r < R; r++)
            {
                std::cout << "[" << r << "]" << std::endl;
                for (auto it = metaRambo[b + B * r].begin(); it != metaRambo[b + B * r].end(); ++it)
                {
                    std::cout << " " << *it;
                }
                {
                    std::cout << "////";
                }
            }
            std::cout << '\n';
        }
    }
}

/*
 *  RAMBO_GPU query to evaluate if query k-mer query_key with length len is present in any document
 */
bitArray RAMBOGPU::queryGpu(std::string singleQueryKmer, int lengthOfSingleQuery)
{
    // test all data with test-fct from MyBloom.cuh
    std::vector<uint> singleHash;
    int posInBait = 0;
    uint barSize = K/8+1;
    int lastCol=-1;

    uint testPosOffs=0;
    uint testsPerBait=0;
    bitArray localBar(K);
    std::vector<uint> testPositions; // saves idx of metaRambos to be tested  (true-positive members)
    std::vector<uint> testBait;  // saves pos and count of idx in test pos

    std::chrono::time_point<std::chrono::high_resolution_clock> tp_start;
    std::chrono::time_point<std::chrono::high_resolution_clock> tp_end;

    // write over/clean/initialise bitarray interim result (bait) and bitarray result (bar, final result) w. time measuring
    tp_start = std::chrono::high_resolution_clock::now();
    GPUEmptyBarAndBait(barSize, barSize*R*B);
    tp_end = std::chrono::high_resolution_clock::now();
    time_emptyingBarAndBait = std::chrono::duration_cast<std::chrono::nanoseconds>(tp_end - tp_start).count();

    // start of time measuring for CPU true-positive member test
    tp_start = std::chrono::high_resolution_clock::now();

    // extract true-positive members/BloomFilter/char-Arrays from the basic structure Rambo_array via hash-function testing
    for (int col = 0; col < R; col++)
    {
        singleHash = myhash(singleQueryKmer, lengthOfSingleQuery, murmurmaxseed, col, range);
        for (int row = 0; row < B; row++)
        {
            // underlying char-Array of Rambo_array at Rambo_array idx row + col * B test
            // singleHash bits are all identically set in Rambo_array -> test positive => returns true
            if ( Rambo_array[row + col * B]->originalTest(singleHash) )
            {

                if (lastCol != col)
                {
                    if(lastCol!=-1)
                    {
                        testBait.push_back(testsPerBait);
                    }

                    lastCol=col;
                    posInBait++;
                    testBait.push_back(testPosOffs);
                    testsPerBait=0;
                }
                testPosOffs++;
                testsPerBait++;
                testPositions.push_back(row+col*B);
                //testPositions.push_back(posInBait);
            }
        }
    }
    testBait.push_back(testsPerBait);
    /*
    std::cout << "TestBait: " << testBait.size() << std::endl;
    int nl=0;
    for (uint i=0; i<testBait.size(); i++) {
        std::cout << i << "=" << testBait[i] << " ";
        nl++;
        if (nl == 2) {
            std::cout << std::endl;
            nl = 0;
        }
    }
    std::cout << std::endl;
    std::cout << "testPositions: " << testPositions.size() << std::endl;
    for (uint i=0; i<testPositions.size(); i++) {
        std::cout << i << "=" << testPositions[i] << std::endl;
    }
    std::cout << std::endl;

    */

    // end of time measuring for CPU true-positive member test
    tp_end = std::chrono::high_resolution_clock::now();
    time_findCandidates = std::chrono::duration_cast<std::chrono::nanoseconds>(tp_end - tp_start).count();

    // calculate size of vectors
    size_testPositions_by = testPositions.size()*sizeof(uint);
    size_testBait_by = testBait.size()*sizeof(uint);

    // copies true-positive vectors from Rambo_array to GPU w. time measuring
    tp_start = std::chrono::high_resolution_clock::now();
    GPUCopyTestPositionsToGPU(testPositions, testBait);
    tp_end = std::chrono::high_resolution_clock::now();
    time_copyCandidatesToGPU = std::chrono::duration_cast<std::chrono::nanoseconds>(tp_end - tp_start).count();

    // union of true-positive members char-arrays (into bitarray interim result - bait) w. time measuring
    tp_start = std::chrono::high_resolution_clock::now();
    GPURunSetBit(posInBait, barSize);
    tp_end = std::chrono::high_resolution_clock::now();
    time_runTests = std::chrono::duration_cast<std::chrono::nanoseconds>(tp_end - tp_start).count();

    // initialise final bitarray result (bar) with first char-array of bait and intersection btw. bar and left over bait w. time measuring
    tp_start = std::chrono::high_resolution_clock::now();
    GPUCombineBaitToBar(barSize, posInBait);
    tp_end = std::chrono::high_resolution_clock::now();
    time_createBar = std::chrono::duration_cast<std::chrono::nanoseconds>(tp_end - tp_start).count();

    // calculate size of bar
    size_barSize_by = barSize * sizeof(char);

    // copies bar from GPU onto CPU and returns it w. time measuring
    tp_start = std::chrono::high_resolution_clock::now();
    localBar = GPUGetResult(K, barSize);
    tp_end = std::chrono::high_resolution_clock::now();
    time_copyResults = std::chrono::duration_cast<std::chrono::nanoseconds>(tp_end - tp_start).count();

    // final result
    return localBar;
}

/*
 *  copies true-positive metarambos to GPU
 */
void RAMBOGPU::copyMRtoGPU(void)
{
    GPUCopyMRtoGPU(range, R, B, (K/8+1), metaRambo, &size_metaRambo_by, &size_metaRamboSizesOffsets_by);
}

/*
 *  cleans up GPU space -> rm CUDA streams, frees CUDA/GPU pointers
 */
void RAMBOGPU::cleanUpMR(void)
{
    GPUCleanupMR();
}

/*
 *  serialises Rambo_array
 */
void RAMBOGPU::serializeRAMBO(std::string dir)
{
    //std::cout << "1. trying to serialize to folder: " << dir << std::endl;

    for (int r = 0; r < R; r++)
    {
        for (int b = 0; b < B; b++)
        {
            //std::cout << "1.1 building Filename... " << std::endl;
            std::string br = dir + std::to_string(b) + "_" + std::to_string(r) + ".txt";
            //std::cout << "1.2 trying to serialize b: " << b << " r: " << r << " in Array: " << (b+B*r) << " to: " << br << std::endl;
            if ((Rambo_array[b + B * r]) != nullptr)
            {
                //std::cout << "BF is NOT NULL " << std::endl;
                (Rambo_array[b + B * r])->serializeBF(br);
            }
            else
            {
                std::cout << "ERR trying to serialize b: " << b << " r: " << r << " in Array: " << (b+B*r) << " to: " << br;
                std::cout << " BF is NULL " << std::endl;
            }
            //std::cout << "serializeRAMBO Done " << std::endl;

        }
    }
}

/*
 *  deserialises Rambo_array from dir
 */
void RAMBOGPU::deserializeRAMBO(std::vector<std::string> dir)
{
    for (int r = 0; r < R; r++)
    {
        for (int b = 0; b < B; b++)
        {
            std::vector<std::string> br;
            for (uint j = 0; j < dir.size(); j++)
            {
                br.push_back(dir[j] + std::to_string(b) + "_" + std::to_string(r) + ".txt");
            }
            Rambo_array[b + B * r]->deserializeBF(br);
        }
    }
}


// given inverted index type arrangement, kmer;files;files;..
void RAMBOGPU::insertion3(std::vector<std::pair<std::string, int>> KeySets)
{
    // make this loop parallel
    //#pragma omp parallel for

    for (uint j = 0; j < KeySets.size(); j++)
    {
        std::vector<uint> hashvals = RAMBOGPU::hashfunc(std::to_string(KeySets[j].second), std::to_string(KeySets[j].second).size()); // R hashvals
        for (int r = 0; r < R; r++)
        {
            std::vector<uint> temp = myhash(KeySets[0].first.c_str(), KeySets[0].first.size(), murmurmaxseed, r, range); // i is the key
            //std::vector<uint> temp = myhash(KeySets[0].first.c_str(), KeySets[0].first.size(), K, r, range); // i is the key
            Rambo_array[hashvals[r] + B * r]->insert(temp);
        }
    }
}

/*
 *  prints metarambos
 */
void RAMBOGPU::printMetaRambo(void)
{
    std::cout << "MeRam[" << metaRambo->size() << "]: ";

    for (int i = 0; i < metaRambo->size(); i++)
    {
        std::cout << (*metaRambo)[i] << ", ";
    }
    std::cout << std::endl;
}


