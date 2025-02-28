//contains all the other non-RAMBO functions
#include <iomanip>
#include <iostream>
#include <fstream>
#include <math.h>
#include <sstream>
#include <string>
#include <string.h>
#include <algorithm>
#include <chrono>
#include "../include/MyBloom.h"
#include "../include/MurmurHash3.h"
#include "../include/utils.h"
#include <map>
#include <cassert>
/*
 *  parses string into array till a fiven delimiter
 */
std::vector<std::string> line2array(std::string line, char d)
{
    std::stringstream is;
    is << line;
    std::vector<std::string> op;
    std::string vals;
    while(getline(is, vals, d))
    {
        op.push_back(vals);
    }
    return op;
}

/*
 *  extracts data from ctx file
 */
std::vector <std::string> getctxdata(std::string filenameSet)
{
    //get the size of Bloom filter by count
    std::ifstream cntfile (filenameSet);
    std::vector <std::string> allKeys;
    int totKmerscnt = 0;
    while (cntfile.good())
    {
        std::string line1;
        while(getline(cntfile, line1))
        {
            std::vector<std::string> linesplit = line2array(line1, ' ');
            allKeys.push_back(linesplit[0]);
            totKmerscnt++;

        }
    }
    std::cout << "total inserted from one file" << filenameSet << ": " << totKmerscnt << std::endl;

    return allKeys;
}

/*
 *  read specific number num of lines from file path
 */
std::vector<std::string> readlines(std::string path, int num)
{
    std::ifstream pathfile(path);
    std::vector <std::string> allfiles;
    int count=0;
    while (pathfile.good())
    {
        std::string line1;
        while(getline(pathfile, line1))
        {
            count++;
            allfiles.push_back(line1);
            if (count > num && num)
            {
                break;
            }
        }
        std::cout << count << '\n';
        return allfiles;
    }
    //TODO: check. inserted to prevent error
    return allfiles;
}

/*
 *  hash function originally from MyBloom.cpp
 */
std::vector<uint> myhash(std::string key, int len, int k, int r, int range)
{
    std::vector <uint> hashvals;
    uint op; // takes 4 byte

    for (int i= 0 + k * r; i < k + k * r; i++)
    {
        MurmurHash3_x86_32(key.c_str(), len, i, &op);
        hashvals.push_back(op%range);
    }
    return hashvals;
}

/*
 *  extracts sequences and their ground truths from file path
 */
std::vector<std::string> getsets(std::string path)
{
    //get the size of Bloom filter by count
    std::ifstream cntfile(path);
    std::vector <std::string> allKeys;
    int linecnt = 0;
    while (cntfile.good())
    {
        std::string line1, vals;
        while(getline(cntfile, line1))
        {
            std::stringstream is;
            is << line1;
            if (linecnt ==0)
            {
                while(getline(is, vals, ' ' ))
                {
                    allKeys.push_back(vals);
                }
            }
            else
            {
                while(getline(is, vals, ' ' ))
                {
                    allKeys.push_back(vals);
                    break;
                }
            }
            linecnt++;
        }
    }
    std::cout << "total lines from this file: " << linecnt-3 << std::endl;
    return allKeys;
}

/*
 *  writes RAMBO result values into file
 */
void writeRAMBOresults(std::string path, int rows, int cols, float* values)
{
    std::ofstream myfile;
    myfile.open (path);
    for (int i = 0; i < rows; i++)
    {
        for (int j = 0; j < cols; j++)
        {
            myfile << std::to_string(values[i*cols +j]) << ",";
        }
        myfile << "\n";
    }
    myfile.close();
}

/*
 *  union between two vectors v1 and v2
 */
std::vector<int> arrayunion(std::vector<int> &v1, std::vector<int> &v2)
{
    std::vector<int> v;
    std::set_union(v1.begin(), v1.end(), v2.begin(), v2.end(),
                   std::back_inserter(v));
    return v;
}

/*
 *  intersection between two vectors v1 and v2
 */
std::vector<int> arrayintersection(std::vector<int> &v1, std::vector<int> &v2)
{
    std::vector<int> v;
    std::set_intersection(v1.begin(), v1.end(), v2.begin(), v2.end(),
                          std::back_inserter(v));
    return v;
}

/*
 *  takes union of two sets
 */
std::set<int> takeunion(std::set<int> set1, std::set<int> set2)
{
    std::set<int> res;
    res.insert(set1.begin(), set1.end());
    res.insert(set2.begin(), set2.end());
    return res;
}

/*
 *  generates random test keys/query k-mers and stores them in vector s
 */
std::vector<std::string> getRandomTestKeys(int keysize, int n)
{
    static const char alphanum[] = "ATGC";
    std::vector<std::string> s;

    for (int j = 0; j < n; ++j)
    {
        std::string st;
        for (int i = 0; i < keysize; ++i)
        {
            st = st + alphanum[rand()%4];
        }
        s.push_back(st);
    }
    return s;
}

/*
 *  creates inverted index
 */
std::map<std::string, std::vector<int>> makeInvIndex(int n, std::vector<std::string> foldernames)
{
    std::map<std::string, std::vector<int>> m;
    for (uint f=0; f<foldernames.size(); f++)
    {
        std::string foldername = foldernames[f];
        for (int batch = 0; batch < 47; batch++)
        {
            std::string dataPath = foldername + std::to_string(batch)+ "_indexed.txt";
            std::vector<std::string> setIDs = readlines(dataPath, 0);
            std::cout << setIDs[0] << std::endl;

            for (uint ss = 0; ss < setIDs.size(); ss++)
            {
                char d = ',';
                std::vector<std::string> setID = line2array(setIDs[ss], d);
                std::string mainfile = foldername + setID[1]+ ".out";
                std::cout << "getting keys" << std::endl;
                std::vector<std::string> keys = getctxdata(mainfile);
                std::cout << "gotkeys" << std::endl;

                if (ss == 0 && batch == 0 && f == 0)
                {
                    for (int i = 0; i < n; i++)
                    {
                        m[keys[i]].push_back(std::stoi(setID[0]));
                    }
                    std::cout << "completed first itr" << std::endl;
                    for(std::map<std::string, std::vector<int> >::iterator it = m.begin(); it != m.end(); ++it)
                    {
                        std::cout << it->first << it->second[0] << "\n";
                    }
                }
                else
                {

                    for (uint i = 0; i < keys.size(); i++)
                    {
                        if (m.find(keys[i]) != m.end())
                        {
                            std::cout << "map contains the key!\n";
                            m[keys[i]].push_back(std::stoi(setID[0]));
                        }
                    }
                }
                std::cout << ss << std::endl;
            }
        }
    }
    return m;
}

/*
 *  splits query query_k into kmersized k-mers and returns them in vector of strings query_kmers
 */
std::vector<std::string> getkmers(std::string query_key, int kmersize)
{
    std::vector<std::string> query_kmers;
    for (uint idx = 0; idx < query_key.size() - kmersize + 1; idx++)
    {
        query_kmers.push_back(query_key.substr(idx, kmersize));
    }
    return query_kmers;
}

