#include <stdio.h>
#include <iostream>
#include <fstream>
#include <cstring>
#include "../include/bitArray.h"

/*
 *  underlying bitArray in RAMBO structure
 *  constructor to initialise char-array with given size
 */
bitArray::bitArray(int size)
{
    ar_size = size;
    A = new char[ar_size/8 +1];
    memset(A,0,(ar_size/8 +1));
//    for (int i = 0; i < (ar_size/8 +1); i++ )
//    {
//        A[i] = '\0';
//    }
}

/*
 *  underlying bitArray in RAMBO structure
 *  constructor to copy another bitArrays values into a new bitArray
 */
bitArray::bitArray(bitArray *ba)
{

    ar_size = ba->ar_size;
    A = new char[ar_size/8 +1];

    // copy the bit array
    for (int i=0; i<(ar_size/8 +1); i++ )
    {
        A[i] = ba->A[i];
    }
}

/*
 *  bitweise OR to set bits in char-array
 */
void bitArray::SetBit(uint k)
{
    A[(k/8)] |= (1 << (k%8));
}

/*
 *  bitweise AND to test for a bit being set (return 1) or unset (return 0)
 */
bool bitArray::TestBit(uint k)
{
    return (A[(k/8)] & (1 << (k%8)));
}

/*
 *  bitweise AND between two char-arrays A and given char-array B and saving the result in char-array A
 */
void bitArray::ANDop(char* B)
{
    for (int i = 0; i < (ar_size/8 +1); i++ )
    {
        A[i] &= B[i];
    }
}

/*
 *  counts and returns set bits in char-array A
 */
int bitArray::getcount(void)
{
    int count = 0;

    for (int kp = 0; kp < ar_size; kp++)
    {
        if (A[(kp/8)] & (1 << (kp%8)))
        {
            count++;
        }
    }
    return count;
}

/*
 * prints out document number where corresponding bits are set true
 */
void bitArray::printKPs(void)
{

    for (int kp = 0; kp < ar_size; kp++)
    {
        if (A[(kp/8)] & (1 << (kp%8)))
        {
            std::cout << kp << " , ";
        }
    }
}

/*
 *  bitweise AND to clear all bits in char-array
 */
void bitArray::ClearBit(uint k)
{
    A[(k/8)] &= ~(1 << (k%8));
}


/*
 *  serialises bitArrays char-array
 */
void bitArray::serializeBitAr(std::string BF_file)
{
    std::ofstream out;
    out.open(BF_file);

    //std::cout << "3. trying to serialize bitArray size: " << (ar_size/8 +1) << std::endl;

    if(!out)
    {
        std::cout << "Cannot open output file" << std::endl;
    }
    out.write(A,ar_size/8 +1);
    out.close();
}

/*
 *  deserialises data from file and saves them into bitArrays char-array
 */
void bitArray::deserializeBitAr(std::vector<std::string> BF_file)
{
    for(uint j = 0; j < BF_file.size(); j++)
    {
        char* C;
        C = new char[ar_size/8 +1];
        std::ifstream in(BF_file[j]);

        if(!in)
        {
            std::cout << "Cannot open input file" << std::endl;
        }

        in.read(C,ar_size/8 +1); //optimise it

        if (j == 0)
        {
            for (int i = 0; i < (ar_size/8 +1); i++)
            {
                A[i] = C[i];
            }
        }
        else
        {
            for (int i = 0; i < (ar_size/8 +1); i++)
            {
                A[i] |= C[i];
            }
        }
        in.close();
        delete[] C;
    }
}

// // Check if SetBit() works:
//
// for ( i = 0; i < 320; i++ )
//    if ( TestBit(A, i) )
//       printf("Bit %d was set !\n", i);
//
// printf("\nClear bit poistions 200 \n");
// ClearBit( A, 200 );
//
// // Check if ClearBit() works:
//
// for ( i = 0; i < 320; i++ )
//    if ( TestBit(A, i) )
//       printf("Bit %d was set !\n", i);
