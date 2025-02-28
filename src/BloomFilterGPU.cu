#include <iostream>
#include <cstring>
#include <chrono>
#include <vector>
#include <math.h>
#include <bitset>
#include <algorithm>

#include "constants.h"
#include "bitArray.h"
#include "MyBloom.h"

#include "BloomFilterGPU.cuh"
#include "deviceFctsGPU.cuh"

/*
 *  underlying BloomFilter in RAMBO structure
 *  constructor to create BloomFilter with given size sz and false-positive rate FPR
 */
BloomFilterGPU::BloomFilterGPU(int sz, float FPR)
{
    p = FPR;
    m_bits = new bitArray(sz);
}

/*
 *  inserts values of a given vector a into BloomFilter
 */
void BloomFilterGPU::insert(std::vector<uint> a)
{
    int N = a.size();

    for (int n = 0; n < N; n++)
    {
        m_bits->SetBit(a[n]);
    }
}

/*
 *  from original MyBloom.cpp
 *  tests if values of a given vector a equal those in BloomFilter
 *  only returns true if all values are equal ("all or nothing")
 */
bool BloomFilterGPU::originalTest(std::vector<uint> a)
{

    // get different data from vector<uint> a
    int N = a.size();

    // original code
    for (int n = 0; n < N; n++)
    {

        // original code
        if (!m_bits->TestBit(a[n]))
        {
            return false;
        }
    }
    return true;
}


// ************************************** not needed *********************************************************

/*
 *  serialises BloomFilter
 */
void BloomFilterGPU::serializeBF(std::string BF_file)
{
    //std::cout << "2. serializing in BF: " << BF_file << std::endl;
    if (m_bits != nullptr)
    {
        //std::cout << "m_bits is NOT NULL " << std::endl;
        m_bits->serializeBitAr(BF_file);
    }
    else
    {
        std::cout << "m_bits is NULL " << std::endl;
    }
}

/*
 *  deserialises BloomFilter from file BF_file
 */
void BloomFilterGPU::deserializeBF(std::vector<std::string> BF_file)
{
    m_bits->deserializeBitAr(BF_file);
}
