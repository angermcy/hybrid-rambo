#ifndef _BLOOMFILTERGPU_
#define _BLOOMFILTERGPU_
#include <vector>
#include "constants.h"
#include <bitset>
#include "bitArray.h"
#include "MyBloom.h"

//std::vector<uint> myhashGPU(std::string key, int len, int k, int r, int range);

class BloomFilterGPU
{
public:

    int n;
    float p;
    int R;
    int k;
    bitArray* m_bits;

    virtual BloomFilterGPU* clone() const
    {
        return new BloomFilterGPU(*this);
    }

    BloomFilterGPU(int sz, float FPR);

    void insert(std::vector<uint> a);
    bool originalTest(std::vector<uint> a);
    void serializeBF(std::string BF_file);
    void deserializeBF(std::vector<std::string> BF_file);

};

#endif
