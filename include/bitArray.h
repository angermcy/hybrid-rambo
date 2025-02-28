#ifndef _INTBITARRAY_
#define _INTBITARRAY_
#include<vector>

/*
 *  header for second underlying layer of Rambo structure
 */
class bitArray
{
public:

    char *A;
    int ar_size;

    bitArray(int size);
    bitArray(bitArray *ba);

    void SetBit(uint k);
    bool TestBit(uint k);
    void ANDop(char* B);

    int getcount(void);
    void printKPs(void);

    void ClearBit(uint k);
    void serializeBitAr(std::string BF_file);
    void deserializeBitAr(std::vector<std::string> BF_file);

};

#endif
