#include <vector>
#include <iostream>
#include <stdio.h>
#include <iomanip>

#include "BloomFilterGPU.cuh"
#include "constants.h"
#include "bitArray.h"
#include "deviceFctsGPU.cuh"
#include "RamboGPU.cuh"
#include "utils.h"
#include "bitComputationGPU.cuh"

/*
 *  global variables
 */

// GPU pointers
int *gpuMRs;
uint *gpuMRSizes;
uint *gpuMROffsets;
char *gpuBait;
char *gpuBar;
uint *gpuTP;
uint *gpuTB;

// CUDA streams
cudaStream_t strm_SetBit;
cudaStream_t strm_SetBarFV;
cudaStream_t strm_SetBarAV;
cudaStream_t strm_CopyBack;
cudaStream_t strm_clnBar;
cudaStream_t strm_clnBait;

cudaStream_t strm_acTP;
cudaStream_t strm_cpTP;
cudaStream_t strm_acTB;
cudaStream_t strm_cpTB;

// for internal tests only
/*
int  *cpuMRs;
uint *cpuMRSizes;
uint *cpuMROffsets;
char *cpuBait;
char *cpuBar;
*/

/*
 *
 */
void GPUCopyMRtoGPU(int n, int rCols, int rRows, int barSize, std::vector<int>* metaRambo, uint *mrSize, uint *mrSizesOffsSize)
{

    //cudaStream_t strm_cpRA;
    cudaStream_t strm_cpMRs;
    cudaStream_t strm_cpMRSizes;
    cudaStream_t strm_cpMROffsets;

    std::vector<uint> mRSizes;
    std::vector<uint> mROffsets;
    std::vector<int> mRs;

    // reset all vectors
    mRs.clear();
    mRSizes.clear();
    mROffsets.clear();

    uint totSize=0;
    int i;
    //int rangeRA = n/8+1;

    //cudaStreamCreate(&strm_cpRA);
    //CudaErr("cudaMalloc/GPUCopyMRtoGPU/gpuRA", cudaMalloc((void **)&gpuRA, rCols*rRows*rangeRA*sizeof(char)));
    //std::cout << "gpuRA size: " << (rCols*rRows*rangeRA*sizeof(char)) << " " << rangeRA << std::endl;

    // copies all metaRambo data into vectors - one big vector
    for (int c = 0; c < rCols; c++)
    {
        for (int r = 0; r < rRows; r++)
        {
            i=r+rRows*c;
            std::copy(metaRambo[i].begin(), metaRambo[i].end(), std::back_inserter(mRs)); // whole metaRambo at idx i copied into mRs
            mRSizes.push_back(metaRambo[i].size()); // size of metaRambo at idx i
            mROffsets.push_back(totSize); //position in bitArray
            //std::cout << "MR: " << c << " r: " << r << " pos: " << (c*rRows)+r << " offs: " << totSize << " size: " << metaRambo[i].size() << std::endl;
            totSize += metaRambo[i].size(); // offset counter

            //std::cout << "gpuRA cp c: " << c << " r: " << r << " pos: " << (c*r*rangeRA*sizeof(char)) << " max: " << (rCols*rRows*rangeRA*sizeof(char)) << std::endl;
            //CudaErr("cudaMemcpyAsync/GPUCopyMRtoGPU/gpuRA", cudaMemcpyAsync(gpuRA+(c*r*rangeRA*sizeof(char)), aBF[c*r]->m_bits->A, rangeRA*sizeof(char), cudaMemcpyHostToDevice, strm_cpRA));

        }
    }

    // calculate sizes of vectors
    *mrSize = totSize * sizeof(int);
    *mrSizesOffsSize = mRSizes.size() * sizeof(uint);

    /*
     *  copies big data values onto GPU
     */
    // create CUDA streams
    cudaStreamCreate(&strm_cpTP);
    cudaStreamCreate(&strm_cpMRs);
    cudaStreamCreate(&strm_cpMRSizes);
    cudaStreamCreate(&strm_cpMROffsets);

    cudaStreamCreate(&strm_SetBit);
    cudaStreamCreate(&strm_SetBarFV);
    cudaStreamCreate(&strm_SetBarAV);
    cudaStreamCreate(&strm_CopyBack);

    cudaStreamCreate(&strm_clnBar);
    cudaStreamCreate(&strm_clnBait);

    // CUDA malloc
    CudaErr("cudaMalloc/GPUCopyMRtoGPU/gpuTB", cudaMalloc((void **)&gpuTB, 2*rCols*sizeof(uint)));  // rCols long but rCols consists of 2 int per entry
    CudaErr("cudaMalloc/GPUCopyMRtoGPU/gpuTP", cudaMalloc((void **)&gpuTP, rCols*rRows*sizeof(uint)));

    CudaErr("cudaMalloc/GPUCopyMRtoGPU/gpuMRs", cudaMalloc((void **)&gpuMRs, totSize * sizeof(int)));
    CudaErr("cudaMalloc/GPUCopyMRtoGPU/gpuMRSizes", cudaMalloc((void **)&gpuMRSizes, mRSizes.size() * sizeof(uint)));
    CudaErr("cudaMalloc/GPUCopyMRtoGPU/gpuMROffsets", cudaMalloc((void **)&gpuMROffsets, mROffsets.size() * sizeof(uint)));

    CudaErr("cudaMalloc/GPUCopyMRtoGPU/gpuBait", cudaMalloc((void **)&gpuBait, barSize*rCols*rRows*sizeof(char))); // bait w. max size "RxBxKi"
    CudaErr("cudaMalloc/GPUCopyMRtoGPU/gpuBar", cudaMalloc((void **)&gpuBar, barSize*sizeof(char)));

    // CUDA memcpyasync
    CudaErr("cudaMemcpyAsync/GPUCopyMRtoGPU/gpuMRs", cudaMemcpyAsync(gpuMRs, mRs.data(), totSize*sizeof(int), cudaMemcpyHostToDevice, strm_cpMRs));
    CudaErr("cudaMemcpyAsync/GPUCopyMRtoGPU/gpuMRSizes", cudaMemcpyAsync(gpuMRSizes, mRSizes.data(), mRSizes.size()*sizeof(uint), cudaMemcpyHostToDevice, strm_cpMRSizes));
    CudaErr("cudaMemcpyAsync/GPUCopyMRtoGPU/gpuMROffsest", cudaMemcpyAsync(gpuMROffsets, mROffsets.data(), mROffsets.size()*sizeof(uint), cudaMemcpyHostToDevice, strm_cpMROffsets));

    // synchronize
    CudaErr("cudaDeviceSynchronize/GPUCopyMRtoGPU", cudaDeviceSynchronize());

    // destroy CUDA streams
    //CudaErr("cudaStreamDestroy/GPUCopyMRtoGPU/strm_cpRA", cudaStreamDestroy(strm_cpRA));
    CudaErr("cudaStreamDestroy/GPUCopyMRtoGPU/strm_cpMRs", cudaStreamDestroy(strm_cpMRs));
    CudaErr("cudaStreamDestroy/GPUCopyMRtoGPU/strm_cpMRSizes", cudaStreamDestroy(strm_cpMRSizes));
    CudaErr("cudaStreamDestroy/GPUCopyMRtoGPU/strm_cpMROffsets", cudaStreamDestroy(strm_cpMROffsets));

    /*
        // FOR EVALUATION IF COPY TO CPU IS CORRECT
        cudaStream_t strm_test1;
        cudaStreamCreate(&strm_test1);

        cpuMRs=(int*)malloc(totSize*sizeof(int));
        CudaErr("tst", cudaMemcpyAsync(cpuMRs, gpuMRs, totSize*sizeof(int), cudaMemcpyDeviceToHost, strm_test1));
        CudaErr("tst", cudaDeviceSynchronize());
        CudaErr("tst", cudaStreamDestroy(strm_test1));

        int z=0;
        for (int i=0; i < totSize; i++ ) {

            if (mRs[i] != cpuMRs[i]) {
                z++;
                std::cout << "pos " << i << " " << mRs[i] << " <> " << cpuMRs[i] << std::endl;
            }
        }
        std::cout << "MRs found " << z << " unidentical ints of " << totSize << std::endl;

        cudaStream_t strm_test2;
        cudaStreamCreate(&strm_test2);
        cudaStream_t strm_test3;
        cudaStreamCreate(&strm_test3);

        cpuMRSizes=(uint*)malloc(mRSizes.size()*sizeof(uint));
        CudaErr("tst", cudaMemcpyAsync(cpuMRSizes, gpuMRSizes, mRSizes.size()*sizeof(uint), cudaMemcpyDeviceToHost, strm_test2));

        cpuMROffsets=(uint*)malloc(mROffsets.size()*sizeof(uint));
        CudaErr("tst", cudaMemcpyAsync(cpuMROffsets, gpuMROffsets, mROffsets.size()*sizeof(uint), cudaMemcpyDeviceToHost, strm_test3));

        CudaErr("tst", cudaDeviceSynchronize());
        CudaErr("tst", cudaStreamDestroy(strm_test2));
        CudaErr("tst", cudaStreamDestroy(strm_test3));

        for (int i=0; i < mRSizes.size(); i++ ) {
            std::cout << "pos " << i << " (";
            std::cout << mRSizes[i] << "=";
            std::cout << cpuMRSizes[i] << ") offs (";
            std::cout << mROffsets[i] << "=";
            std::cout << cpuMROffsets[i] << ")" << std::endl;
        }

        cpuBait = (char *)malloc(barSize*rCols*rRows* sizeof(char));
        cpuBar = (char *)malloc(barSize * sizeof(char));
        */

}

/*
 *  initalises the GPU pointers with '\0'
 */
void GPUEmptyBarAndBait(int barSize, int baitSize)
{

    CudaErr("cudaMemsetAsync/GPUCopyMRtoGPU/gpuBar", cudaMemsetAsync(gpuBar, '\0', barSize*sizeof(char), strm_clnBar));
    CudaErr("cudaMemsetAsync/GPUCopyMRtoGPU/gpuBait", cudaMemsetAsync(gpuBait, '\0', baitSize*sizeof(char), strm_clnBait));
    CudaErr("cudaDeviceSynchronize/GPUCopyMRtoGPU", cudaDeviceSynchronize());

    // TEST
    /*
    memset(cpuBar, '\0', barSize*sizeof(char) );
    memset(cpuBait, '\0', baitSize*sizeof(char) );
    */
}

/*
 *  copies all test data onto the GPU
 */
void GPUCopyTestPositionsToGPU(std::vector<uint>& tp, std::vector<uint>& tb)
{
    CudaErr("cudaMemcpyAsync/GPUCopyTestPositionsToGPU/gpuTP", cudaMemcpyAsync(gpuTP,
            tp.data(),
            tp.size()*sizeof(uint),
            cudaMemcpyHostToDevice, strm_cpTP));

    CudaErr("cudaMemcpyAsync/GPUCopyTestPositionsToGPU/gpuTB", cudaMemcpyAsync(gpuTB,
            tb.data(),
            tb.size()*sizeof(uint),
            cudaMemcpyHostToDevice, strm_cpTB));
    CudaErr("cudaDeviceSynchronize/GPUCopyTestPositionsToGPU/gpuTP", cudaDeviceSynchronize());
    /*
        uint *cpuPos;
        cpuPos = (uint *)malloc(tp.size()*sizeof(uint));
        cudaStream_t strm_test2;
        cudaStreamCreate(&strm_test2);

        CudaErr("cudaMemcpyAsync/GPUCopyTestPositionsToGPU/gpuTP", cudaMemcpyAsync(cpuPos,
                                                                                   gpuTP,
                                                                                   tp.size()*sizeof(uint),
                                                                                   cudaMemcpyDeviceToHost, strm_test2));
        CudaErr("cudaDeviceSynchronize/GPUCopyTestPositionsToGPU/gpuTP", cudaDeviceSynchronize());

        CudaErr("tst", cudaStreamDestroy(strm_test2));

        for (uint i=0; i < tp.size()/2; i++ ) {

            std::cout << "size (" << tp[i*2] << "=" << cpuPos[i*2] << ") (" << tp[i*2+1] << "=" << cpuPos[i*2+1] << ")" << std::endl;
        }
    */
}

/*
 *  destroys all used streams and pointers
 */
void GPUCleanupMR(void)
{
    CudaErr("cudaStreamDestroy/GPUCleanupMR/streamMRs", cudaStreamDestroy(strm_SetBit));
    CudaErr("cudaStreamDestroy/GPUCleanupMR/strm_SetBarFV", cudaStreamDestroy(strm_SetBarFV));
    CudaErr("cudaStreamDestroy/GPUCleanupMR/strm_SetBarFV", cudaStreamDestroy(strm_SetBarAV));
    CudaErr("cudaStreamDestroy/GPUCleanupMR/strm_CopyBack", cudaStreamDestroy(strm_CopyBack));

    CudaErr("cudaFree/GPUCleanupMR/gpuMRs", cudaFree(gpuMRs));
    CudaErr("cudaFree/GPUCleanupMR/gpuMRSizes", cudaFree(gpuMRSizes));
    CudaErr("cudaFree/GPUCleanupMR/gpuMROffsets", cudaFree(gpuMROffsets));

    CudaErr("cudaStreamDestroy/GPUCopyMRtoGPU/strm_clnBar", cudaStreamDestroy(strm_clnBar));
    CudaErr("cudaStreamDestroy/GPUCopyMRtoGPU/strm_clnBait", cudaStreamDestroy(strm_clnBait));
}

/*
 *  calculates the column-wise union of char-arrays (Bloom Filter for Union) to compromise the data
 *  SetBitAllGpu: bitwise OR
 */
void GPURunSetBit(uint numBaits, uint barSize)
{
    dim3 gridSize(1);
    dim3 blockSize(numBaits); //no of threads

    SetBitAllGpu<<<gridSize, blockSize>>>(barSize, gpuTB, gpuTP, gpuBait, gpuMRs, gpuMRSizes, gpuMROffsets);
    CudaErr("cudaDeviceSynchronize/GPURunSetBit2", cudaDeviceSynchronize());
}

/*
 *  sets the first value for bitarray result (bar) with first char-array of bitarray interim results (bait)
 *  calculates intersection of bar and left over char-arrays from bait
 *  result: char-array (1:= documents at this position in metarambo potentially contains query k-mer)
 */
void GPUCombineBaitToBar(uint barSize, uint baitCols)
{
    SetBarFirstValueGpu<<<1, 1, 0, strm_SetBarFV>>>(gpuBar, gpuBait, barSize);
    CudaErr("cudaDeviceSynchronize/GPUGetResult 1", cudaDeviceSynchronize());

    dim3 gridSize(1);
    dim3 blockSize(baitCols); //no of threads

    AndOpAllGpu<<<gridSize, blockSize>>>(gpuBar, gpuBait, baitCols, barSize);
    CudaErr("cudaDeviceSynchronize/GPUGetResult 2", cudaDeviceSynchronize());
}


/*
 *  saves GPU calculated result in bitArray and returns bitArray
 */
bitArray GPUGetResult(uint K, uint barSize)
{
    bitArray resBar(K);

    /*  TEST FCT TO RETURN BITARRAY
        char *x;
        char *y;

        x=(char*)malloc(baitCols*barSize*sizeof(char));
        y=(char*)malloc(barSize*sizeof(char));

        CudaErr("16 - cudaMemcpyAsync/(resBar.A", cudaMemcpyAsync(x, gpuBait, baitCols*barSize*sizeof(char), cudaMemcpyDeviceToHost, strm_CopyBack));
        CudaErr("cudaDeviceSynchronize/GPUGetResult 3", cudaDeviceSynchronize());

        int z=0;
        for (int i=0; i < baitCols*barSize*sizeof(char); i++ ) {
            if (z >= barSize) {
                z=0;
                std::cout << std::endl;
            }
            z++;
            std::cout << atoi( &x[i] ) << ", ";
        }

        // reduce bait to bitarray
        std::cout << "SetBarFirst: " << barSize << std::endl;
    */


    CudaErr("cudaMemcpyAsync/(resBar.A", cudaMemcpyAsync(resBar.A, gpuBar, barSize*sizeof(char), cudaMemcpyDeviceToHost, strm_CopyBack));
    CudaErr("cudaDeviceSynchronize/GPUGetResult 3", cudaDeviceSynchronize());

    return resBar;
}


/*
 *  prints out last CUDA errors with specific text
 */
void CudaErrorPrint(std::string txt)
{
    cudaError_t cudaError;

    cudaError = cudaGetLastError();
    if (cudaError != cudaSuccess)
    {
        std::cout << txt << ", last cuda error: " << cudaGetErrorString(cudaError) << std::endl;
    }
    /*else
    {
        std::cout << txt << ", no error." << std::endl;
    }*/

}

/*
 *  prints out function specific CUDA error with specific text
 */
void CudaErr(std::string msg, cudaError_t cudaError)
{
    if (cudaError != cudaSuccess)
    {
        std::cout << "CUDA ERROR at " << msg << ": " << cudaGetErrorString(cudaError) << std::endl;
    }
}

