#ifndef _INTDEVICEFCTS_
#define _INTDEVICEFCTS_

#include<vector>

#include "bitArray.h"

/*
 *  device/GPU/CUDA functions
 */

__global__ void SetBitAllGpu(uint barSize, uint *tb, uint *tp, char *bait, int *mr, uint *mrSize, uint *mrOffs);
__global__ void SetBarFirstValueGpu(char *bar, char *bait, int bitarrayLength);
__global__ void AndOpAllGpu(char *bar, char *bait, int ramboCols, int bitarrayLength);


#endif
