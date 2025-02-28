#include <stdio.h>
#include <iostream>
#include <fstream>

#include "bitArray.h"
#include "deviceFctsGPU.cuh"

/*
 *  union of all true document char-arrays per column into interium result bait
 *  params: barSize, testBait, testPositions, bait, mR, mRSizes, mROffsets (from GPUCopyMRtoGPU in bitComputationGPU)

         example:
         testBait size is 2*r (maxColumns from MetaRambo)
         0 -> start     - thread 1 r=0
         3 -> offset    - thread 1 r=0
         3 -> start     - thread 2 r=1
         4 -> offset    - thread 2 r=1
         7 -> start     - thread 3 r=2
         3 -> offset    - thread 3 r=2
         ...

         testPositions
         5 - candidate 1 for thread 1     (start idx = 0)
         7
         9
         17 - candidate 1 for thread 2    (start idx = 3)
         18
         22
         24
         36 - candidate 1 for thread 3    (start idx = 7)
         39
         40
         ...
 */
__global__ void SetBitAllGpu(uint barSize, uint *tb, uint *tp, char *bait, int *mr, uint *mrSize, uint *mrOffs)
{
    int i = threadIdx.x; // this replaces the for loop  ->  no of threadIdx = numBaits

    //for (uint i = 0; i <= numBaits; i++)
    for (uint t=tb[i*2]; t<tb[i*2]+tb[i*2+1]; t++)
    {
        // for test purposes
        //printf("bait pos loop t=%d posinTP0: %d PosInTP1: %d  bp0: %d bp1: %d Offs=%d Sizefs=%d \n", t,
        //       tp[t*2],
        //       tp[t*2],
        //       mrOffs[ tp[ t*2 ] ],
        //       mrSize[ tp[ t*2 ] ]
        //       );

        for (uint s = 0; s < mrSize[ tp[ t ] ]; s++)
        {
            bait[(i*barSize) + (mr[mrOffs[ tp[ t ] ] + s] / 8)] |= (1 << (mr[mrOffs[ tp[ t ] ] + s] % 8));
        }
    }
    //}

    return;
}


/*
 *  initialise bitarray result (bar) with first char-Array of bitarray interim result (bait)
 */
__global__ void SetBarFirstValueGpu(char *bar, char *bait, int bitarrayLength)
{
    for (int len = 0; len < bitarrayLength; len++)
    {
        bar[len] = bait[len]; //+ 1];
    }
}


/*
 *  intersection/AND-join of all interium result char-arrays (per column) into result bar
 */
__global__ void AndOpAllGpu(char *bar, char *bait, int baitCols, int bitarrayLength)
{
    int baitNumber = threadIdx.x; // replaces the loop
    //for (int baitNumber = index; baitNumber < baitCols; baitNumber+=stride)
    //{
    for (int len = 0; len < bitarrayLength; len++)
    {
        bar[len] &= bait[bitarrayLength * baitNumber + len]; //+ 1];
    }
    //}
}
