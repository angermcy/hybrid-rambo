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
#include "MyBloom.h"
#include "MurmurHash3.h"
#include "Rambo_construction.h"
#include "utils.h"
#include "constants.h"
#include "bitArray.h"
#include <set>
#include <iterator>
#include <bitset>

/*
 *  constructor for basic RAMBO structure
 *  constructs pointer array of BloomFilter with given dimension R x B
 */
RAMBO::RAMBO(int n, int r1, int b1, int Ki)
{
    R = r1;
    B = b1;
    K = Ki;

    range = n;
    k = 3;

    Rambo_array = new BloomFiler*[B*R];

    metaRambo = new std::vector<int>[B*R];
    for(int b = 0; b < B; b++)
    {
        for(int r = 0; r < R; r++)
        {
            Rambo_array[b + B*r] = new BloomFiler(range, p);
        }
    }

}

/*
 *  calculates hash values with given string key and given length len via MurmurHash3
 */
std::vector<uint> RAMBO::hashfunc(std::string key, int len)
{
    std::vector<uint> hashvals;
    uint op;
    for (int i = 0; i < R; i++)
    {
        MurmurHash3_x86_32(key.c_str(), len, 10, &op); //seed i
        hashvals.push_back(op%B);
    }
    return hashvals;
}

/*
 *  reads out data from a file and saves them in vector allKeys
 */
std::vector<std::string> RAMBO::getdata(std::string filenameSet)
{
    //get the size of Bloom filter by count
    std::ifstream cntfile (filenameSet);
    std::vector <std::string> allKeys;
    int totKmerscnt = 0;
    while (cntfile.good())
    {
        std::string line1, vals;
        while(getline(cntfile, line1))
        {
            std::stringstream is;
            is << line1;
            if (line1[0]!= '>' && line1.size() > 30 )
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
void RAMBO::createMetaRambo(int K, bool verbose)
{
    for(int i = 0; i < K; i++)
    {
        std::vector<uint> hashvals = RAMBO::hashfunc(std::to_string(i), std::to_string(i).size()); // R hashvals, each with max value B


        for(int r = 0; r < R; r++)
        {
            metaRambo[hashvals[r] + B*r].push_back(i);
        }
    }

    //print RAMBO meta deta
    if (verbose)
    {
        for(int b = 0; b < B; b++)
        {
            for(int r = 0; r < R; r++)
            {
                for (auto it = metaRambo[b + B*r].begin(); it != metaRambo[b + B*r].end(); ++it)
                {
                    std::cout << " " << *it;
                }

                std::cout << " | " << std::endl;
            }
            std::cout << std::endl;
        }
    }
}

/*
 *  inserts hash values of vector of strings keys in Rambo_array at position of hash values of string setID
 */
void RAMBO::insertion(std::string setID, std::vector<std::string> keys)
{
    std::vector<uint> hashvals = RAMBO::hashfunc(setID, setID.size());

    //make this loop parallel
    #pragma omp parallel for
    for(std::size_t i = 0; i < keys.size(); ++i)
    {
        for(int r = 0; r < R; r++)
        {
            std::vector<uint> temp = myhash(keys[i].c_str(), keys[i].size(), k,r, range);
            Rambo_array[hashvals[r] + B*r]->insert(temp);
        }
    }
}

/*
 *  restores the BloomFilterArray from Rambo_array
 *  otherwise error during multiple testing
 */
/*
void RAMBO::restoreBloomFilterArray()
{
   for (int b = 0; b < B; b++)
   {
       for (int r = 0; r < R; r++)
       {
           Rambo_array[b + B * r] = Rambo_array_save[b + B * r]->clone();
       }
   }
}
*/

/*
 *  extend of insertion-function by including reading data string setID and vector of strings keys from vector alllines
 */
void RAMBO::insertion2(std::vector<std::string> alllines)
{
    //make this loop parallel
    //#pragma omp parallel for
    for(std::size_t i = 0; i < alllines.size(); ++i)
    {
        char d = ';';

        // read query k-mers
        std::vector<std::string>KeySets = line2array(alllines[i], d);

        // read query ground truths
        std::vector<std::string>KeySet = line2array(KeySets[1], ',');

        // insert query k-mers into basic RAMBO structure Rambo_array with idx determined by hash-function (for later testing)
        for (uint j = 0; j < KeySet.size(); j++)
        {
            std::vector<uint> hashvals = RAMBO::hashfunc(KeySet[j], KeySet[j].size());
            for(int r = 0; r < R; r++)
            {
                std::vector<uint> temp = myhash(KeySets[0].c_str(), KeySets[0].size(), k, r, range);
                Rambo_array[hashvals[r] + B*r]->insert(temp);
            }
        }
    }
}

/*
 *  RAMBO_CPU query to evaluate if query k-mer query_key with length len is present in any document
 */
bitArray RAMBO::query (std::string query_key, int len)
{
    bitArray bitarray_K(K);
    std::vector<uint> check;
    for(int r = 0; r < R; r++)
    {
        check = myhash(query_key, len, k, r,range);  //hash values correspondign to the keys
        bitArray bitarray_K1(K);
        for(int b = 0; b < B; b++)
        {
            if (Rambo_array[b + B*r]->test(check))
            {
                //std::cout << "TESTING col: " << r << " row: " << b << " size: " << metaRambo[b + B*r].size() << std::endl;
                for (uint j = 0; j < metaRambo[b + B*r].size(); j++)
                {
                    bitarray_K1.SetBit(metaRambo[b + B*r][j]);
                }
            }
        }
        if (r == 0)
        {
            bitarray_K = bitarray_K1;
        }
        else
        {
            bitarray_K.ANDop(bitarray_K1.A);
        }
    }
    std::vector<uint>().swap(check);
    return bitarray_K;
}

/*
 *  serialises Rambo_array
 */
void RAMBO::serializeRAMBO(std::string dir)
{
    for(int b = 0; b < B; b++)
    {
        for(int r = 0; r < R; r++)
        {
            std::string br = dir + std::to_string(b) + "_" + std::to_string(r) + ".txt";
            Rambo_array[b + B*r]->serializeBF(br);
        }
    }
}

/*
 *  deserialises Rambo_array from dir
 */
void RAMBO::deserializeRAMBO(std::vector<std::string> dir)
{
    for(int b = 0; b < B; b++)
    {
        for(int r = 0; r < R; r++)
        {
            std::vector<std::string> br;
            for (uint j = 0; j < dir.size(); j++)
            {
                br.push_back(dir[j] + std::to_string(b) + "_" + std::to_string(r) + ".txt");
            }
            Rambo_array[b + B*r]->deserializeBF(br);

        }
    }
}

// // give set and keys in the set
// void RAMBO::insertionRare (std::string setID, std::vector<std::string> keys){
//   vector<uint> hashvals = RAMBO::hashfunc(setID, setID.size()); // R hashvals
//
//   //make this loop parallel
//   int skip =0;
//   #pragma omp parallel for
//   for(std::size_t i=0; i<keys.size(); ++i){
//     bitArray MemVec = RAMBO::query(keys[i].c_str(), keys[i].size());
//     if ( MemVec.getcount() <10 ){
//       for(int r=0; r<R; r++){
//       vector<uint> temp = myhash(keys[i].c_str(), keys[i].size() , k, r, range);
//         Rambo_array[hashvals[r] + B*r]->insert(temp);
//     }
//   }
//     else{ skip++;}
//   }
//   cout<<"skipped "<<to_string(skip)<<endl;
// }
