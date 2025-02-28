#include <iomanip>
#include <iostream>
#include <algorithm>
#include <fstream>
#include <math.h>
#include <sstream>
#include <string>
#include <string.h>
#include <algorithm>
#include <chrono>
#include "../include/MyBloom.h"
#include "../include/MurmurHash3.h"
#include "../include/Rambo_construction.h"
#include "../include/RamboGPU.cuh"
#include "../include/utils.h"
#include "../include/constants.h"
#include "../include/bitArray.h"
#include <ctime>
#include <chrono>
#include <unistd.h>
#include <thread>

/*
 *  prints hints for parameters on cmd
 */
void showRunHint(void)
{
    std::cout << std::endl << "usage: rambo 0 {options} {parameters} " << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "\t-insertCPU       " << std::endl;
    std::cout << "\t-insertGPU       " << std::endl;
    std::cout << "\t-ser             " << std::endl;
    std::cout << "\t-testCPUlog      " << std::endl;
    std::cout << "\t-testCPUtime     " << std::endl;
    std::cout << "\t-testGPUlog      " << std::endl;
    std::cout << "\t-testGPUtime     " << std::endl;
    std::cout << "\t-deserCPU        " << std::endl;
    std::cout << "\t-deserGPU        " << std::endl << std::endl;
    std::cout << "Parameters:" << std::endl;

    std::cout << "\t-K=nn " << std::endl;
    std::cout << "\t-R=nn " << std::endl;
    std::cout << "\t-B=nn " << std::endl;
    std::cout << "\t-cardinality=nnnn " << std::endl;
    std::cout << "\t-lfsuff=\"sometext\" " << std::endl;
    std::cout << "\t-testrepeat=nn" << std::endl;
    std::cout << "\t-testsets=nn" << std::endl;


    std::cout << "example: rambo 0 -insertCPU -testCPUlog -testrepeat=5" << std::endl << std::endl;
}

/*
 *  main fct
 */
int main(int argc, char** argv)
{
    bool logToFile      = false;
    bool insertCPU      = false;
    bool insertGPU      = false;
    bool ser            = false;
    bool testCPUlog     = false;
    bool testCPUtime    = false;
    bool testGPUlog     = false;
    bool testGPUtime    = false;

    bool deserCPU       = false;
    bool deserGPU       = false;

    int n_perSet = 1000000000; //cardinality of each set

    // with default init in case no parameters where given
    int R_all = 2;
    int B_all = 15;
    int K = 100;

    uint testSetSize = 1000; // as in original run 1000 tests

    //times
    float time_deseril_ms = 0.0f;


    std::string lfn = "rambo";
    std::string lfsuff = "";

    // shows hint automatically when less than three arguments are used
    if (argc < 3)
    {
        showRunHint();
        return 0;
    }

    const std::vector<std::string> args(argv+1, argv+argc); // convert C-style to modern C++

    // extract all parameters from given arguments
    for (std::string a : args )
    {
        for (uint j = 0; j < a.length(); j++)
        {
            a[j] = std::tolower(a[j]);
        }
        if (a =="-logfile")       logToFile = true;
        if (a =="-insertcpu")    insertCPU = true;
        if (a =="-insertgpu")    insertGPU = true;
        if (a =="-ser")          ser = true;

        if (a =="-testcpulog")   testCPUlog = true;
        if (a =="-testcputime")  testCPUtime = true;

        if (a =="-testgpulog")   testGPUlog = true;
        if (a =="-testgputime")  testGPUtime = true;

        if (a =="-desercpu")    deserCPU = true;
        if (a =="-desergpu")    deserGPU = true;
        if (a.rfind("-k=", 0) == 0)
        {
            std::string v = a.substr(3);
            K = std::stoi(v);
        }
        if (a.rfind("-r=", 0) == 0)
        {
            std::string v = a.substr(3);
            R_all = std::stoi(v);
        }
        if (a.rfind("-b=", 0) == 0)
        {
            std::string v = a.substr(3);
            B_all = std::stoi(v);
        }
        if (a.rfind("-lfsuff=", 0) == 0)
        {
            lfsuff = a.substr(8);
        }
        if (a.rfind("-cardinality=", 0) == 0)
        {
            std::string v = a.substr(13);
            n_perSet = std::stoi(v);
        }
        if (a.rfind("-testsets=", 0) == 0)
        {
            std::string v = a.substr(10);
            testSetSize = std::stoi(v);
        }

    }

    /** backup cout buffer and redirect tofilet **/
    //auto *bkpCoutBuf = std::cout.rdbuf();

    // automatic name generation for log files
    if (logToFile)
    {
        if (testCPUlog)
            lfn += "_CPU-log_K";
        if (testCPUtime)
            lfn += "_CPU-time_K";

        if (testGPUlog)
            lfn += "_GPU-log_K";
        if (testGPUtime)
            lfn += "_GPU-time_K";

        lfn += std::to_string(K);
        lfn += "_R";
        lfn += std::to_string(R_all);
        lfn += "_B";
        lfn += std::to_string(B_all);
        lfn += "_TS";
        lfn += std::to_string(testSetSize);
        lfn += lfsuff;
        lfn += ".log";

        std::cout << "using logfile: " << lfn << std::endl;

        freopen(lfn.c_str(), "w", stdout);
    }

    // prints out parameter values in current run
    std::cout << "************* Parameters of run *************" << std::endl;
    std::cout << std::boolalpha << std::endl;
    std::cout << "insertCPU       = " << insertCPU << std::endl;
    std::cout << "insertGPU       = " << insertGPU << std::endl;
    std::cout << "ser             = " << ser << std::endl;
    std::cout << "testCPUlog      = " << testCPUlog << std::endl;
    std::cout << "testCPUtime     = " << testCPUtime << std::endl;
    std::cout << "testGPUlog      = " << testGPUlog << std::endl;
    std::cout << "testGPUtime     = " << testGPUtime << std::endl;
    std::cout << "deserCPU        = " << deserCPU << std::endl;
    std::cout << "deserGPU        = " << deserGPU << std::endl << std::endl << std::endl;
    std::cout << std::noboolalpha;

    std::cout << "set cardinality = " << n_perSet << std::endl;
    std::cout << "R               = " << R_all << std::endl;
    std::cout << "B               = " << B_all << std::endl;
    std::cout << "K               = " << K << std::endl;
    std::cout << "***************************************************************** " << std::endl;


    float ins_time;
    //float ins_time_total;
    // constructors for CPU and hybrid RAMBO
    RAMBO myRambo(n_perSet, R_all, B_all, K);
    RAMBOGPU RamboGPU(n_perSet, R_all, B_all, K);

    //  details of RAMBO set partitioning
    myRambo.createMetaRambo (K, false);
    RamboGPU.createMetaRambo(K, false);
    std::cout << "created meta" << std::endl;

    //insert into RAMBO
    std::string job(argv[1]);
    std::string SerOpFile ="data/results/RAMBO_Ser" + std::to_string(K) + 'R' + std::to_string(R_all) + 'B' + std::to_string(B_all) + '_' + job + '/';
    std::string comand = "mkdir -p ";
    comand += SerOpFile;
    system(comand.c_str());

    /********
     *  insert into hybrid RAMBO
     ********/

    if (insertGPU == true)
    {
        std::cout << "************* Starting insert GPU *************" << std::endl;

        //log files
        std::ofstream failedFiles;
        failedFiles.open("logFileToy_"+ std::to_string(K) + '_' + job + ".txt");
        int stpCnt = 0;

        // sliding window used to divide longer sequences into parts
        int kh = K/100;

        if (K < 100)
        {
            kh = 1;
        }

        std::chrono::time_point<std::chrono::high_resolution_clock> t5 = std::chrono::high_resolution_clock::now();
        for (int batch = 0; batch < kh; batch++)
        {
            // extract document names from collection
            std::chrono::time_point<std::chrono::high_resolution_clock> t3 = std::chrono::high_resolution_clock::now();

            std::string dataPath = "data/"+ job + "/" + std::to_string(batch) + "_indexed.txt";
            std::vector<std::string> setIDs = readlines(dataPath, 0);
            int stpt;
            stpt = 5;
            int allsize = 0;
            int nofrecs = setIDs.size();

            if (K<100)
            {
                nofrecs = K;
            }

            // insert documents into basic RAMBO structure myRambo
            for (uint ss = 0; ss < nofrecs; ss++)
            {
                stpCnt++;
                char d = ',';
                std::vector<std::string> setID = line2array(setIDs[ss], d); // setID might be every sequence string in Docs
                std::string mainfile = "data/" + job + "/inflated/" + setID[1] + ".out";
                std::cout << ss << " ";
                std::vector<std::string> keys = getctxdata(mainfile); // keys might be no. behind every sequence string in Docs
                failedFiles << mainfile << " " << keys.size() << std::endl;
                allsize += keys.size();

                if (keys.size() == 0)
                {
                    std::cout << mainfile << " does not exists or empty " << std::endl;
                    failedFiles << mainfile << " does not exists or empty " << std::endl;
                }

                RamboGPU.insertion(setID[0], keys);

            }
            std::chrono::time_point<std::chrono::high_resolution_clock> t4 = std::chrono::high_resolution_clock::now();
            std::cout << std::chrono::duration_cast<std::chrono::nanoseconds>(t4-t3).count()/1000000000.0 << "sec" << std::endl;
            ins_time = (t4-t3).count()/1000000000.0;

            std::cout << "***************************************************************************" << std::endl;
            std::cout << "Processing Batch " << batch << " ends with " << setIDs.size() << " inserts and ";
            std::cout << allsize << " bytes " << (allsize/1024) << " kB " << ((allsize/1024)/1024) << " MB in ";
            std::cout << ins_time << "seconds." << std::endl;
            std::cout << "***************************************************************************" << std::endl;

            failedFiles << "insertion time (sec) of 100 files: " << ins_time << std::endl;
            std::cout << "************* END of insertGPU batch " << batch << " *************" << std::endl;

        }
        std::chrono::time_point<std::chrono::high_resolution_clock> t6 = std::chrono::high_resolution_clock::now();
        ins_time = (t6-t5).count()/1000000000.0;
        std::cout << "Import finished in " << ins_time/60 << "minutes." << std::endl;

        std::cout << "************* END of insertGPU *************" << std::endl;

        //serialize myRambo under results
        if (ser)
        {
            std::cout << "Serializing RAMBO at: " << SerOpFile << std::endl;
            RamboGPU.serializeRAMBO(SerOpFile);
            //gives number of 1s in 30 BFs
            for (int kpp = 0; kpp < (B_all*R_all); kpp++)
            {
                std::cout << "packing: " << RamboGPU.Rambo_array[kpp]->m_bits->getcount() << std::endl;
            }
        }

    }


    /********
     *  insert into CPU RAMBO
     ********/
    if (insertCPU == true)
    {
        std::cout << "************* Starting insert CPU *************" << std::endl;
        //log files

        std::ofstream failedFiles;
        failedFiles.open("logFileToy_" + std::to_string(K) + '_' + job + ".txt");
        int stpCnt = 0;
        // sliding window used to divide longer sequences into parts
        int kh = K/100;

        if (K < 100)
        {
            kh = 1;
        }

        std::chrono::time_point<std::chrono::high_resolution_clock> t5 = std::chrono::high_resolution_clock::now();
        for (int batch = 0; batch < kh; batch++)
        {
            // extract document names from collection
            std::cout << "batch " << batch << " K " << K << " K/100 " << (K/100) << std::endl;
            std::chrono::time_point<std::chrono::high_resolution_clock> t3 = std::chrono::high_resolution_clock::now();

            std::string dataPath = "data/"+ job + "/" + std::to_string(batch) + "_indexed.txt";
            std::cout << "***************************************************************************" << std::endl;
            std::cout << "Processing Batch " << batch << " with file: " << dataPath << std::endl;
            std::cout << "***************************************************************************" << std::endl;
            std::vector<std::string> setIDs = readlines(dataPath, 0);

            int allsize = 0;
            uint nofrecs = setIDs.size();

            if (K<100)
            {
                nofrecs = K;
            }

            // insert documents into basic RAMBO structure myRambo
            for (uint ss = 0; ss < nofrecs; ss++)
            {
                stpCnt++;
                char d = ',';
                std::vector<std::string> setID = line2array(setIDs[ss], d);
                std::string mainfile = "data/" + job + "/inflated/" + setID[1]+ ".out";
                std::cout << ss << " ";
                std::vector<std::string> keys = getctxdata(mainfile);
                failedFiles << mainfile << " : " << keys.size() << std::endl;
                allsize += keys.size();
                if (keys.size() == 0)
                {
                    std::cout << mainfile << " does not exists or empty " << std::endl;
                    failedFiles << mainfile << " does not exists or empty " << std::endl;
                }

                myRambo.insertion(setID[0], keys);

            }
            std::chrono::time_point<std::chrono::high_resolution_clock> t4 = std::chrono::high_resolution_clock::now();
            std::cout << std::chrono::duration_cast<std::chrono::nanoseconds>(t4-t3).count()/1000000000.0 << "sec" << std::endl;
            ins_time = (t4-t3).count()/1000000000.0;

            std::cout << "***************************************************************************" << std::endl;
            std::cout << "Processing Batch " << batch << " ends with " << setIDs.size() << " inserts and ";
            std::cout << allsize << " bytes " << (allsize/1024) << " kB " << ((allsize/1024)/1024) << " MB in ";
            std::cout << ins_time << "seconds." << std::endl;
            std::cout << "***************************************************************************" << std::endl;

            failedFiles << "insertion time (sec) of 100 files: " << ins_time << std::endl;
            std::cout << "************* END of insertCPU batch " << batch << " *************" << std::endl;

        }
        std::chrono::time_point<std::chrono::high_resolution_clock> t6 = std::chrono::high_resolution_clock::now();
        ins_time = (t6-t5).count()/1000000000.0;
        std::cout << "Import finished in " << ins_time/60 << "minutes." << std::endl;

        std::cout << "************* END of insertCPU *************" << std::endl;
        //serialize myRambo
        if (ser)
        {
            std::cout << "Serializing RAMBO at: " << SerOpFile << std::endl;

            myRambo.serializeRAMBO(SerOpFile);
            //gives number of 1s in 30 BFs
            for (int kpp=0; kpp<(R_all * B_all); kpp++)
            {
                std::cout << "packing: " << myRambo.Rambo_array[kpp]->m_bits->getcount() << std::endl;
            }
        }
    }

    /********
     *  CPU-version: deserialize from file into myRambo
     ********/
    if (deserCPU == true)
    {
        std::cout << "************* Starting deserialization *************" << std::endl;

        std::chrono::time_point<std::chrono::high_resolution_clock> tp_start;
        std::chrono::time_point<std::chrono::high_resolution_clock> tp_end;

        tp_start = std::chrono::high_resolution_clock::now();

        std::vector<std::string> SerOpFile2;
        SerOpFile2.push_back(SerOpFile); // multiple files can be pushed here

        myRambo.deserializeRAMBO(SerOpFile2);
        tp_end = std::chrono::high_resolution_clock::now();

        time_deseril_ms = std::chrono::duration_cast<std::chrono::nanoseconds>(tp_end-tp_start).count()/1000000.0;
        std::cout << "desealized in " << time_deseril_ms << " msecs." << std::endl;

    }

    /********
     *  hybrid version: deserialize from file into myRambo
     ********/
    if (deserGPU == true)
    {
        std::cout << "************* Starting deserialization *************" << std::endl;

        std::chrono::time_point<std::chrono::high_resolution_clock> tp_start;
        std::chrono::time_point<std::chrono::high_resolution_clock> tp_end;

        tp_start = std::chrono::high_resolution_clock::now();

        std::vector<std::string> SerOpFile2;
        SerOpFile2.push_back(SerOpFile); // multiple files can be pushed here

        RamboGPU.deserializeRAMBO(SerOpFile2);

        tp_end = std::chrono::high_resolution_clock::now();

        time_deseril_ms = std::chrono::duration_cast<std::chrono::nanoseconds>(tp_end-tp_start).count()/1000000.0;

        std::cout << "desealized in " << time_deseril_ms << " msecs." << std::endl;
    }

    /********
     *  test queries on CPU RAMBO with log file
     ********/
    if(testCPUlog == true)
    {
        // repeat test for no. of tests nTests

        // read query k-mers from file
        std::cout << "************* Starting test CPU run *************" << std::endl;
        std::string fname = "data/ArtfcKmersToy" + std::to_string(testSetSize) + "_" + std::to_string(K) + ".txt";
        std::cout << "reading file " << fname << std::endl;

        std::vector<std::string> alllines = readlines("data/ArtfcKmersToy" + std::to_string(K) + ".txt", 0);
        std::vector<std::string> testKeys;
        std::vector<int> gt_size;

        for(uint i = 0; i < alllines.size(); i++)
        {
            std::vector<std::string>KeySets = line2array(alllines[i], ';');
            testKeys.push_back(KeySets[0]);
            gt_size.push_back( line2array(KeySets[1], ',').size() );
        }

        // sort K documents into B different groups
        //myRambo.restoreBloomFilterArray();
        myRambo.createMetaRambo (K, false);
        std::cout << "total number of queries : " << testKeys.size() << std::endl;

        // insert query k-mers into basic structure myRambo (for later testing)
        myRambo.insertion2(alllines);
        std::cout << "loaded test keys" << std::endl;

        // create FPtestFileToy
        float fp=0;
        std::ofstream FPtestFile;
        FPtestFile.open("FPtestFileToy.txt");

        // start time measuring
        std::clock_t t5_cpu = std::clock();
        std::chrono::time_point<std::chrono::high_resolution_clock> t5 = std::chrono::high_resolution_clock::now();

        // variables used for printing
        int memVecCount = 0;
        int ks = testKeys.size();

        // testing all query k-mers if present in documents
        for (std::size_t i=0; i<testKeys.size(); i++)
        {
            bitArray MemVec = myRambo.query(testKeys[i], testKeys[i].size());
            memVecCount = MemVec.getcount();

            // print query k-mer, number of documents they are present, ground truth size and document numbers
            std::cout << "Key: " << testKeys[i] << " pos= " << i << " of " << ks;
            std::cout << " DocCounter: " << memVecCount;
            std::cout << " gt_size: " << gt_size[i];
            std::cout << " Docs: ";
            MemVec.printKPs();
            std::cout << std::endl;

            // calculate false positive rate
            fp = fp + (memVecCount)*0.1/((K - gt_size[i])*0.1);

        }

        // end time measuring
        std::clock_t t6_cpu = std::clock();
        std::chrono::time_point<std::chrono::high_resolution_clock> t6 = std::chrono::high_resolution_clock::now();

        // print false positive rate and write it into FPtestFile
        std::cout << "fp rate is: " << fp/(testKeys.size());
        FPtestFile << "fp rate is: " << fp/(testKeys.size());
        std::cout << std::endl;

        // calculate query times and print them
        float QTpt_cpu = 1000.0 * (t6_cpu-t5_cpu)/(CLOCKS_PER_SEC*testKeys.size()); //in ms
        float QTpt = std::chrono::duration_cast<std::chrono::nanoseconds>(t6-t5).count()/(1000000.0*testKeys.size());
        float totalTime = std::chrono::duration_cast<std::chrono::nanoseconds>(t6-t5).count()/1000000.0;
        std::cout << "total time for #queries = " << testKeys.size() << " : " << totalTime << " milliseconds\n" << std::endl;
        std::cout << "query time wall clock is : " << QTpt << ", cpu is :" << QTpt_cpu << " milliseconds\n\n";
        FPtestFile << "query time wall clock is : " << QTpt << ", cpu is :" << QTpt_cpu << " milliseconds\n\n";

        std::cout << "************* END test CPU run *************" << std::endl;

    }

    /********
     *  test queries on CPU RAMBO without log file - focus on time measuring
     ********/
    if(testCPUtime == true)
    {
        float fp=0.0;
        float time_loadtestkeys = 0.0f;
        float time_copymalloc = 0.0f;
        float time_processing = 0.0f;
        float time_copyback = 0.0f;
        float time_free = 0.0f;
        float time_createMR = 0.0f;
        float time_insertion = 0.0f;

        std::chrono::time_point<std::chrono::high_resolution_clock> tp_start;
        std::chrono::time_point<std::chrono::high_resolution_clock> tp_end;

        // read query k-mers from file
        std::cout << "************* Starting test CPU run *************" << std::endl;
        std::string fname = "data/ArtfcKmersToy" + std::to_string(testSetSize) + "_" + std::to_string(K) + ".txt";

        std::cout << "reading file " << fname << std::endl;

        tp_start = std::chrono::high_resolution_clock::now();

        std::vector<std::string> alllines = readlines(fname, 0);
        std::vector<std::string> testKeys;
        std::vector<int> gt_size;

        for(uint i = 0; i < alllines.size(); i++)
        {
            std::vector<std::string>KeySets = line2array(alllines[i], ';');
            testKeys.push_back(KeySets[0]);
            gt_size.push_back( line2array(KeySets[1], ',').size() );
        }
        tp_end = std::chrono::high_resolution_clock::now();
        time_loadtestkeys += std::chrono::duration_cast<std::chrono::nanoseconds>(tp_end - tp_start).count()/1000000.0;

        // sort K documents into B different groups
        // this is only for repeated tests so ignore it whe ntaking processtimes
        //myRambo.restoreBloomFilterArray();
        tp_start = std::chrono::high_resolution_clock::now();
        myRambo.createMetaRambo (K, false);
        tp_end = std::chrono::high_resolution_clock::now();
        time_createMR += std::chrono::duration_cast<std::chrono::nanoseconds>(tp_end - tp_start).count()/1000000.0;

        std::cout << "total number of queries : " << testKeys.size() << std::endl;
        // ------------

        tp_start = std::chrono::high_resolution_clock::now();

        // insert query k-mers into basic structure myRambo (for later testing)
        myRambo.insertion2(alllines);
        tp_end = std::chrono::high_resolution_clock::now();
        time_insertion = std::chrono::duration_cast<std::chrono::nanoseconds>(tp_end - tp_start).count()/1000000.0;

        std::cout << "test keys loaded" << std::endl;

        // start time measuring
        tp_start = std::chrono::high_resolution_clock::now();
        // testing all query k-mers if present in documents
        for (std::size_t i=0; i<testKeys.size(); i++)
        {
            //std::cout << "TESTING key: " << testKeys[i] << std::endl;
            bitArray MemVec = myRambo.query(testKeys[i], testKeys[i].size());
            // calculate false positive rate
            fp = fp + (MemVec.getcount())*0.1/((K - gt_size[i])*0.1);
        }
        tp_end = std::chrono::high_resolution_clock::now();
        time_processing = std::chrono::duration_cast<std::chrono::nanoseconds>(tp_end - tp_start).count()/1000000.0;

        std::cout << "fp rate is:             " << fp/(testKeys.size()) << std::endl;
        std::cout << "no of tests:            " << testSetSize << std::endl;

        std::cout << "Loading Testkeys:       " << time_loadtestkeys << " msecs" << std::endl;
        std::cout << "Creating MR:            " << time_createMR << " msecs" << std::endl;
        std::cout << "Insertion2:             " << time_insertion << " msecs" << std::endl;

        std::cout << "copy MR to GPU:         " << 0 << " msecs" << std::endl;
        std::cout << "Emptying Bar and Bait:  " << 0 << " msecs" << std::endl;
        std::cout << "Finding Candidates:     " << 0 << " msecs" << std::endl;
        std::cout << "Copy Candidates to GPU: " << 0 << " msecs" << std::endl;
        std::cout << "Run Tests:              " << time_processing << " msecs" << std::endl;
        std::cout << "Create Bar:             " << 0 << " msecs" << std::endl;
        std::cout << "Copy Bar to CPU:        " << 0 << " msecs" << std::endl;
        std::cout << "Free and cleanup:       " << 0 << " msecs" << std::endl;
        std::cout << "Deserialisation:        " << time_deseril_ms << " msecs" << std::endl;

        float total = time_loadtestkeys + time_createMR + time_insertion + time_processing;

        std::cout << "is in total:            " << total << " msecs" << std::endl;

        std::cout << "CSV, CPU, " << K << ", " << R_all << ", " << B_all << ", " << testSetSize;
        std::cout  << ", " << fp/(testKeys.size());
        std::cout << ", " << total;
        std::cout << ", " << time_loadtestkeys;
        std::cout << ", " << time_createMR;
        std::cout << ", " << time_insertion;
        std::cout << ", " << 0;
        std::cout << ", " << 0;
        std::cout << ", " << 0;
        std::cout << ", " << 0;
        std::cout << ", " << time_processing;
        std::cout << ", " << 0;
        std::cout << ", " << 0;
        std::cout << ", " << 0;
        std::cout << ", " << time_deseril_ms;
        std::cout << std::endl;

        std::cout << "************* END test CPU run *************" << std::endl;


    }

    /********
     *  test queries on hybrid RAMBO with log file - version 1
     ********/
    if(testGPUlog == true)
    {
        float fp=0;
        // read query k-mers from file
        std::cout << "************* Starting test GPU logging run *************" << std::endl;
        std::string fname = "data/ArtfcKmersToy" + std::to_string(testSetSize) + "_" + std::to_string(K) + ".txt";
        std::cout << "reading file " << fname << std::endl;

        std::vector<std::string> alllines = readlines("data/ArtfcKmersToy" + std::to_string(K) + ".txt", 0);
        std::vector<std::string> testKeys;
        std::vector<int> gt_size;

        for(uint i = 0; i < alllines.size(); i++)
        {
            std::vector<std::string>KeySets =  line2array(alllines[i], ';');
            testKeys.push_back(KeySets[0]);
            gt_size.push_back( line2array(KeySets[1], ',').size() );
        }

        // sort K documents into B different groups
        //RamboGPU.restoreBloomFilterArray();
        RamboGPU.createMetaRambo (K, false);
        std::cout << "total number of queries: " << testKeys.size() << std::endl;

        // insert query k-mers into basic structure myRambo (for later testing)
        RamboGPU.insertion2 (alllines);
        std::cout << "test keys loaded" << std::endl;

        // start time measuring
        std::clock_t t5_cpu = std::clock();
        std::chrono::time_point<std::chrono::high_resolution_clock> t5 = std::chrono::high_resolution_clock::now();

        RamboGPU.copyMRtoGPU();

        // variables used for printing
        int memVecCount = 0;
        int ks=testKeys.size();

        uint size_TestBait = 0;
        uint size_TestPosi = 0;
        uint size_BarSize = 0;

        // testing all query k-mers if present in documents
        for (std::size_t i = 0; i < ks; i++)
        {
            //bitArray MemVec = RamboGPU.queryGpuForLogging(testKeys[i], testKeys[i].size());
            bitArray MemVec = RamboGPU.queryGpu(testKeys[i], testKeys[i].size());

            memVecCount = MemVec.getcount();

            // print query k-mer, number of documents they are present, ground truth size and document numbers
            std::cout << "Key: " << testKeys[i] << " pos= " << i << " of " << ks;
            std::cout << " DocCounter: " << memVecCount;
            std::cout << " gt_size: " << gt_size[i];
            std::cout << " Docs: ";
            MemVec.printKPs();
            std::cout << std::endl;
            fp += memVecCount*0.1/((K - gt_size[i])*0.1);
            //collect sizes
            size_TestBait += RamboGPU.size_testBait_by;
            size_TestPosi += RamboGPU.size_testPositions_by;
            size_BarSize += RamboGPU.size_barSize_by;
        }

        // end time measuring
        std::clock_t t6_cpu = std::clock();
        std::chrono::time_point<std::chrono::high_resolution_clock> t6 = std::chrono::high_resolution_clock::now();

        std::cout << "fp rate is: " << fp/(testKeys.size()) << std::endl;

        RamboGPU.cleanUpMR();

        // calculate query times and print them
        float QTpt_cpu = 1000.0 * (t6_cpu-t5_cpu)/(CLOCKS_PER_SEC * testKeys.size()); //in ms
        float QTpt = std::chrono::duration_cast<std::chrono::nanoseconds>(t6-t5).count()/(1000000.0*testKeys.size());
        float totalTime = std::chrono::duration_cast<std::chrono::nanoseconds>(t6-t5).count()/1000000.0;
        std::cout << "total time for " << testKeys.size() << " queries: " << totalTime << " milliseconds\n" << std::endl;
        std::cout << "query time wall clock is: " << QTpt << ", cpu is:" << QTpt_cpu << " milliseconds\n\n";

        uint totsiz = RamboGPU.size_metaRambo_by + (2 * RamboGPU.size_metaRamboSizesOffsets_by) + size_TestBait + size_TestPosi;
        std::cout << "CSVSIZE, GPU, " << RamboGPU.size_metaRambo_by << ", " << (2 * RamboGPU.size_metaRamboSizesOffsets_by) << ", " << size_TestBait << ", " << size_TestPosi << ", " << totsiz << ", " << size_BarSize << std::endl;


        std::cout << "************* END test GPU run *************" << std::endl;


    }

    /********
     *  test queries on hybrid RAMBO without log file - focus on overall time measuring - version 1
     ********/
    if(testGPUtime == true)
    {
        float fp=0;

        float time_cpMRToGPU = 0.0f;
        float time_createMR = 0.0f;
        float time_insertion = 0.0f;
        float time_loadtestkeys = 0.0f;
        float time_copymalloc = 0.0f;
        float time_processing = 0.0f;
        float time_copyback = 0.0f;
        float time_free = 0.0f;
        float time_emptyingBarAndBait = 0.0f;
        float time_findCandidates = 0.0f;
        float time_copyCandidatesToGPU = 0.0f;
        float time_runTests  = 0.0f;
        float time_createBar = 0.0f;


        std::chrono::time_point<std::chrono::high_resolution_clock> tp_start;
        std::chrono::time_point<std::chrono::high_resolution_clock> tp_end;

        // read query k-mers from file
        std::cout << "************* Starting test GPU detailed timing run *************" << std::endl;
        std::string fname = "data/ArtfcKmersToy" + std::to_string(testSetSize) + "_" + std::to_string(K) + ".txt";
        std::cout << "reading file " << fname << std::endl;

        tp_start = std::chrono::high_resolution_clock::now();

        std::vector<std::string> alllines = readlines(fname, 0);
        std::vector<std::string> testKeys;
        std::vector<int> gt_size;

        for(uint i = 0; i < alllines.size(); i++)
        {
            std::vector<std::string>KeySets =  line2array(alllines[i], ';');
            testKeys.push_back(KeySets[0]);
            gt_size.push_back( line2array(KeySets[1], ',').size() );
        }

        tp_end = std::chrono::high_resolution_clock::now();
        time_loadtestkeys += std::chrono::duration_cast<std::chrono::nanoseconds>(tp_end - tp_start).count()/1000000.0;

        tp_start = std::chrono::high_resolution_clock::now();
        RamboGPU.createMetaRambo (K, false);
        tp_end = std::chrono::high_resolution_clock::now();
        time_createMR += std::chrono::duration_cast<std::chrono::nanoseconds>(tp_end - tp_start).count()/1000000.0;

        std::cout << "total number of queries: " << testKeys.size() << std::endl;

        // insert query k-mers into basic structure myRambo (for later testing)
        tp_start = std::chrono::high_resolution_clock::now();
        RamboGPU.insertion2 (alllines);
        tp_end = std::chrono::high_resolution_clock::now();
        time_insertion += std::chrono::duration_cast<std::chrono::nanoseconds>(tp_end - tp_start).count()/1000000.0;

        tp_start = std::chrono::high_resolution_clock::now();
        RamboGPU.copyMRtoGPU();
        tp_end = std::chrono::high_resolution_clock::now();
        time_cpMRToGPU += std::chrono::duration_cast<std::chrono::nanoseconds>(tp_end - tp_start).count()/1000000.0;

        std::cout << "test keys loaded." << std::endl;
        uint size_TestBait = 0;
        uint size_TestPosi = 0;
        uint size_BarSize = 0;

        // testing all query k-mers if present in documents
        for (std::size_t i = 0; i < testKeys.size(); i++)
        {
            //std::cout << "TESTING key: " << testKeys[i] << std::endl;
            bitArray MemVec = RamboGPU.queryGpu(testKeys[i], testKeys[i].size());

            // calculate false positive rate
            fp += MemVec.getcount()*0.1/((K - gt_size[i])*0.1);
            time_emptyingBarAndBait += RamboGPU.time_emptyingBarAndBait;
            time_findCandidates += RamboGPU.time_findCandidates;
            time_copyCandidatesToGPU += RamboGPU.time_copyCandidatesToGPU;
            time_runTests += RamboGPU.time_runTests;
            time_createBar += RamboGPU.time_createBar;
            time_copyback += RamboGPU.time_copyResults;
            //collect sizes
            size_TestBait += RamboGPU.size_testBait_by;
            size_TestPosi += RamboGPU.size_testPositions_by;
            size_BarSize += RamboGPU.size_barSize_by;
        }

        // free space (GPU pointer), destroy CUDA streams
        tp_start = std::chrono::high_resolution_clock::now();
        RamboGPU.cleanUpMR();
        tp_end = std::chrono::high_resolution_clock::now();
        time_free += std::chrono::duration_cast<std::chrono::nanoseconds>(tp_end - tp_start).count()/1000000.0;

        // end time measuring
        // // calculate query times and print them
        std::cout << "fp rate is:             " << fp/(testKeys.size()) << std::endl;
        std::cout << "no of tests:            " << testSetSize << std::endl;

        std::cout << "Loading Testkeys:       " << time_loadtestkeys << " msecs" << std::endl;
        std::cout << "Creating MR:            " << time_createMR << " msecs" << std::endl;
        std::cout << "Insertion2:             " << time_insertion << " msecs" << std::endl;
        std::cout << "copy MR to GPU:         " << time_cpMRToGPU << " msecs" << std::endl;
        std::cout << "Emptying Bar and Bait:  " << time_emptyingBarAndBait/1000000.0 << " msecs" << std::endl;
        std::cout << "Finding Candidates:     " << time_findCandidates/1000000.0 << " msecs" << std::endl;
        std::cout << "Copy Candidates to GPU: " << time_copyCandidatesToGPU/1000000.0 << " msecs" << std::endl;
        std::cout << "Run Tests:              " << time_runTests/1000000.0 << " msecs" << std::endl;
        std::cout << "Create Bar:             " << time_createBar/1000000.0 << " msecs" << std::endl;
        std::cout << "Copy Bar to CPU:        " << time_copyback/1000000.0 << " msecs" << std::endl;
        std::cout << "Free and cleanup:       " << time_free << " msecs" << std::endl;
        std::cout << "Deserialisation:        " << time_deseril_ms << " msecs" << std::endl;
        float total = time_loadtestkeys + time_createMR + time_insertion + time_cpMRToGPU +
                      (time_emptyingBarAndBait/1000000.0) + (time_findCandidates/1000000.0) + (time_copyCandidatesToGPU/1000000.0) +
                      (time_runTests/1000000.0) + (time_createBar/1000000.0) + (time_copyback/1000000.0) + time_free;

        std::cout << "is in total:            " << total << " msecs" << std::endl;

        uint totsiz = RamboGPU.size_metaRambo_by + (2 * RamboGPU.size_metaRamboSizesOffsets_by) + size_TestBait + size_TestPosi;

        std::cout << "MetaRambo to GPU:                        " << RamboGPU.size_metaRambo_by << " bytes" << std::endl;
        std::cout << "MetaRambo Sizes- and Offssetlists to GPU: " << (2 * RamboGPU.size_metaRamboSizesOffsets_by) << " bytes" << std::endl;
        std::cout << "TestBaits to GPU:                        " << size_TestBait << " bytes" << std::endl;
        std::cout << "TestPositions to GPU:                    " << size_TestPosi << " bytes" << std::endl;

        std::cout << "Result from GPU:                         " << size_BarSize << " bytes" << std::endl;

        std::cout << "Total to GPU:                            " << totsiz << " bytes" << std::endl;

        std::cout << "CSVTIME, GPU, " << K << ", " << R_all << ", " << B_all << ", " << testSetSize;
        std::cout  << ", " << fp/(testKeys.size());
        std::cout << ", " << total;
        std::cout << ", " << time_loadtestkeys;
        std::cout << ", " << time_createMR;
        std::cout << ", " << time_insertion;
        std::cout << ", " << time_cpMRToGPU;
        std::cout << ", " << time_emptyingBarAndBait/1000000.0;
        std::cout << ", " << time_findCandidates/1000000.0;
        std::cout << ", " << time_copyCandidatesToGPU/1000000.0;
        std::cout << ", " << time_runTests/1000000.0;
        std::cout << ", " << time_createBar/1000000.0;
        std::cout << ", " << time_copyback/1000000.0;
        std::cout << ", " << time_free;
        std::cout << ", " << time_deseril_ms;
        std::cout << std::endl;

        std::cout << "CSVSIZE, GPU, " << RamboGPU.size_metaRambo_by << ", ";
        std::cout << (2 * RamboGPU.size_metaRamboSizesOffsets_by) << ", ";
        std::cout << size_TestBait << ", ";
        std::cout << size_TestPosi << ", ";
        std::cout << totsiz << ", ";
        std::cout << size_BarSize << std::endl;

        //FPtestFile << "query time wall clock is: " << QTpt << ", cpu is:" << QTpt_cpu << " milliseconds\n\n";

        std::cout << "************* END test detailed timing GPU run *************" << std::endl;

    }

    std::cout << "************************** rambo finished. **************************" << std::endl;

    // exit main
    return 0;
}


