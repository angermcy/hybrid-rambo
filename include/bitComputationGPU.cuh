#ifndef _BITCOMPUTEGPU_
#define _BITCOMPUTEGPU_

#include <string>
#include <string.h>
#include <vector>
#include <stdio.h>

#include "bitArray.h"
#include "constants.h"
#include "deviceFctsGPU.cuh"
#include "BloomFilterGPU.cuh"
#include "RamboGPU.cuh"
#include "utils.h"


/*
 *  callers for all device/GPU functions
 */

//void GPUCopyMRtoGPU(int n, int rCols, int rRows, int barSize, std::vector<int>* metaRambo);
void GPUCopyMRtoGPU(int n, int rCols, int rRows, int barSize, std::vector<int>* metaRambo, uint *mrSize, uint *mrSizesOffsSize);

void GPUEmptyBarAndBait(int barSize, int baitSize);
void GPUCopyTestPositionsToGPU(std::vector<uint>& tp, std::vector<uint>& tb);

void GPUCleanupMR(void);

void GPURunSetBit(uint numBaits, uint barSize);
void GPUCombineBaitToBar(uint barSize, uint baitCols);

bitArray GPUGetResult(uint K,uint barSize);

void CudaErrorPrint(std::string txt);
void CudaErr(std::string msg, cudaError_t cudaError);


#endif
