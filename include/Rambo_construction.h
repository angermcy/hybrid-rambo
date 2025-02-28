#ifndef _RamboConstruction_
#define _RamboConstruction_
#include <vector>
#include <set>
#include <string>
#include <bitset>
#include "../include/constants.h"
#include "../include/bitArray.h"
#include "../include/MyBloom.h"

/*
 *  header for overall outer layer of Rambo structure
 */
class RAMBO
{
public:

    int R ;
    int B;
    int K;
    int n;
    float p;
    int range;
    int k;
    float FPR;

    BloomFiler** Rambo_array;

    std::vector<int>* metaRambo;

    RAMBO(int n, int r1, int b1, int Ki);

    std::vector<uint> hashfunc( std::string key, int len);
    std::vector <std::string> getdata(std::string filenameSet);
    void createMetaRambo(int K, bool verbose);

    void insertion (std::string setID, std::vector<std::string> keys);
    void restoreBloomFilterArray(void);
    void insertion2 (std::vector<std::string> alllines);

    bitArray query (std::string query_key, int len);

    void serializeRAMBO(std::string dir);
    void deserializeRAMBO(std::vector<std::string> dir);

    // no fct exists
    std::set<int> takeunion(std::set<int> set1, std::set<int> set2);
    std::set<int> takeIntrsec(std::set<int>* setArray);
    bitArray queryseq (std::string query_key, int len);
    void insertionRare (std::string setID, std::vector<std::string> keys);


};

#endif
