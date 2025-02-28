#include "../include/MurmurHash3.h"
#include <iostream>
#include <cstring>
#include <chrono>
#include <vector>
#include "../include/MyBloom.h"
#include <math.h>
#include "../include/constants.h"
#include <bitset>
#include "../include/bitArray.h"


/*
 *  underlying BloomFilter in RAMBO structure
 *  constructor to create BloomFilter with given size sz and false-positive rate FPR
 */
BloomFiler::BloomFiler(int sz, float FPR)
{
    p = FPR;
    m_bits = new bitArray(sz);
}

/*
 *  inserts values of a given vector a into BloomFilter
 */
void BloomFiler::insert(std::vector<uint> a)
{
    int N = a.size();
    for (int n = 0; n < N; n++)
    {
        m_bits->SetBit(a[n]);
    }
}

/*
 *  tests if values of a given vector a equal those in BloomFilter
 *  only returns true if all values are equal ("all or nothing")
 */
bool BloomFiler::test(std::vector<uint> a)
{
    int N = a.size();
    for (int n = 0; n < N; n++)
    {
        if (!m_bits->TestBit(a[n]))
        {
            return false;
        }
    }
    return true;
}

/*
 *  serialises BloomFilter
 */
void BloomFiler::serializeBF(std::string BF_file)
{
    m_bits->serializeBitAr(BF_file);
}

/*
 *  deserialises BloomFilter from file BF_file
 */
void BloomFiler::deserializeBF(std::vector<std::string> BF_file)
{
    m_bits->deserializeBitAr(BF_file);
}
