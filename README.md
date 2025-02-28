# Imprambo
adaption of original RAMBO README.md (https://github.com/gaurav16gupta/RAMBO_MSMT)

## download requirements/must install
- python 3.8.X (tested w. python 3.8.13)
- parallel
- wget
- bzip2
- cortexpy (for download look at: https://cortexpy.readthedocs.io/en/latest/overview.html#installation)
- gcc 11.2 version (not working with gcc 12+)

## run 
1. Unzip data from folder 0 : unzip data/0.zip
2. download data: sh data/0/download.sh (may need writing requirements on Linux distribution, takes some time)
3. Create directories: mkdir -p results/RAMBOSer_100_0 results/RAMBOSer_200_0 results/RAMBOSer_500_0 results/RAMBOSer_1000_0 results/RAMBOSer_2000_0

4. generate artifical query k-mers for testing: python3 data/new_artificialKmer.py
	edit value of variable testsets (default value: 1000) to generate desired number of query k-mers
	
5. edit Makefile
    if you use Windows, comment the part with #Windows above in and comment the part with #Linux out
6. Compile code: make
7. Run program
Version  7.1: run script file for all combinations of K = {100, 200, 400, 600,  800}, R = {2, 4, 6, 8}, B = {5, 10, 15, 20} (used for tests)
	
	./ramboIter.sh
		
Version 7.2: choose parameters for run manually
   ./build/rambo 0 -X -P
   
   with X being:
   - insertCPU: builds basic structure for running RAMBO_CPU; necessary for "testCPUlog" and "testCPUtime"
   - testCPUlog: runs queries with RAMBO_CPU and creates a log file with timer, false-positive rate and documents which potentially contain the specific query as output; prior "insertCPU" mandatory
   - testCPUtime: runs queries with RAMBO_CPU with timer and false-positve rate as only outputs (used for test runs); prior "insertCPU" mandatory
   - insertGPU: builds basic structure for running RAMBO_Hybrid (uses both CPU and GPU); necessary for "testGPUlog" and "testGPUtime"
   - testGPUlog: runs queries with RAMBO_Hybrid and creates a log file with timer, false-positive rate and documents which potentially contain the specific query as output; prior "insertGPU" mandatory
   - testGPUtime: runs queries with RAMBO_Hybrid with timer and false-positve rate as only outputs (used for test runs); prior "insertGPU" mandatory
   - ser: serializes the basic RAMBO structure
   	->  has to be used in combination with "insertCPU" or "insertGPU"
   - deserCPU:  deserializes file for RAMBO_CPU
   - deserGPU: deserializes file for RAMBO_Hybrid
   
   with P being:
   - logfile:  saves outputs  in a log file automatically 
   - K=N: choose number of documents N to be searched in (default value: 100)
   - R=N: choose number of independent hash-functions N; accuracy of search (default value: 2)
   - B=N: choose number of groups N in which documents are sorted (default value: 15)
   - cardinality=N: choose cardinality N (default value: 1000000000)
   - lfsuff=S: choose suffix S to append to logfile name
   - testsets=N: choose number of queries (default value: 1000) 
   	-> make sure to have generated artificalKmer-file with this number of query k-mers prior

(last updated: Fri., 22/07/29)
