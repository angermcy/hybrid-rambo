#ifndef _MYBLOOM_
#define _MYBLOOM_
#include <vector>
#include "../include/constants.h"
#include <bitset>
#include "../include/bitArray.h"

/*
 *  header for first underlying layer of Rambo structure
 */
class BloomFiler
{
public:

    int n;
    float p;
    int R;
    bitArray* m_bits;


    BloomFiler(int sz, float FPR);

    virtual BloomFiler* clone() const
    {
        return new BloomFiler(*this);
    }

    void insert(std::vector<uint> a);
    bool test(std::vector<uint> a);

    void serializeBF(std::string BF_file);
    void deserializeBF(std::vector<std::string> BF_file);


};

#endif
