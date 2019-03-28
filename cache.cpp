/*
Copyright (c) 2015-2018 Justin Funston

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

   3. This notice may not be removed or altered from any source
   distribution.
*/

#include <bitset>
#include <cassert>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <iterator>

#include "cache.h"
#include "misc.h"
// Kmeans
/* Cluster */
#include "/aenao-99/karyofyl/dkm-master/include/dkm_parallel.hpp"
/* Local */
// #include "/home/vic/Documents/dkm-master/include/dkm_parallel.hpp"


using namespace std;

bool enable_prints = 0;
bool enable_prints_file = 0;

/* Cluster */
bool local = false;
/* Local */
// bool local = true;

Cache::Cache(unsigned int num_lines, unsigned int assoc) : maxSetSize(assoc)
{
   assert(num_lines % assoc == 0);
   // The set bits of the address will be used as an index
   // into sets. Each set is a deque containing "assoc" items
   sets.resize(num_lines / assoc);
}

// Given the set and tag, return the cache line's state
// Invalid and "not found" are equivalent
CacheState Cache::findTag(uint64_t set, uint64_t tag) const
{
   for (auto it = sets[set].cbegin(); it != sets[set].cend(); ++it) {
      if (it->tag == tag) {
         return it->state;
      }
   }

   return CacheState::Invalid;
}

// Changes the cache line specificed by "set" and "tag" to "state"
// The cache only saves lines that are not Invalid, so delete if that
// is the new state
void Cache::changeState(uint64_t set, uint64_t tag, CacheState state)
{
   for (auto it = sets[set].begin(); it != sets[set].end(); ++it) {
      if (it->tag == tag) {
         if (state == CacheState::Invalid) {
            sets[set].erase(it);
            break;
         } else {
            it->state = state;
            break;
         }
      }
   }
}

// A complete LRU is mantained for each set, using the ordering
// of the set deque. The end is considered most
// recently used. The specified line must be in the cache, it 
// will be moved to the most-recently used position
void Cache::updateLRU(uint64_t set, uint64_t tag)
{
#ifdef DEBUG
   CacheState foundState = findTag(set, tag);
   assert(foundState != CacheState::Invalid);
#endif

   CacheLine temp;
   auto it = sets[set].begin();
   for(; it != sets[set].end(); ++it) {
      if(it->tag == tag) {
         temp = *it;
         break;
      }
   } 

   sets[set].erase(it);
   sets[set].push_back(temp);
}

// Called if a new cache line is to be inserted. Checks if
// the least recently used line needs to be written back to
// main memory.
bool Cache::checkWriteback(uint64_t set, uint64_t& tag) const
{
   if (sets[set].size() < maxSetSize) {
      // There is room in the set, it does not matter the state of the
      // LRU line
      return false;
   }

   const CacheLine& evict = sets[set].front();
   tag = evict.tag;
   return (evict.state == CacheState::Modified || evict.state == CacheState::Owned);
}

// Insert a new cache line by popping the least recently used line if necessary
// and pushing the new line to the back (most recently used)
void Cache::insertLine(uint64_t set, uint64_t tag, CacheState state, std::array<int,64> data)
{
   if (sets[set].size() == maxSetSize) {
      sets[set].pop_front();
   }
   // std::cout << "Max Set Size" << maxSetSize << "\n";

   sets[set].emplace_back(tag, state, data);
}

// Updating cache data in case of a hit
void Cache::updateData(uint64_t set, uint64_t tag, std::array<int,64> data)
{
    for (auto it = sets[set].begin(); it != sets[set].end(); ++it) {
        if (it->tag == tag) {
            for (int j=0; j<64; j++) {
                it->data[j] = data[j];
            }
        }
   }
}

std::string Cache::outfile_generation(std::string function, const std::string method, const std::string suite, const std::string benchmark, const std::string size, const int entries, \
    const int bits_ignored, const int updates, const std::string state, const int binary_file)
{
    std::string file_path;

    auto updates_str = std::to_string(updates);
    auto entries_str = std::to_string(entries);
    auto bits_ignored_str = std::to_string(bits_ignored);

    if (function == "tableUpdate") {
        if (binary_file) {
            file_path = "/aenao-99/karyofyl/results/mcs/" + suite + "/" + benchmark + "/" + size + "/compressibility/(" + entries_str + ")" + state + "_" + updates_str + "_binary.out";
        }
        else {
            file_path = "/aenao-99/karyofyl/results/mcs/" + suite + "/" + benchmark + "/" + size + "/compressibility/(" + entries_str + ")" + state + "_" + updates_str + ".out";
        }
    }
    else if (function == "modifyData") {
        if (binary_file) {
            file_path = "/aenao-99/karyofyl/results/mcs/" + suite + "/" + benchmark + "/" + size + "/compressibility/(" + entries_str + ")" + state + "_" + method + "_" + updates_str + "_binary.out";
        }
        else {
            file_path = "/aenao-99/karyofyl/results/mcs/" + suite + "/" + benchmark + "/" + size + "/compressibility/(" + entries_str + ")" + state + "_" + method + "_" + updates_str + ".out";
        }
    }
    else if (function == "printSimilarity") {
        file_path = "/aenao-99/karyofyl/results/mcs/" + suite + "/" + benchmark + "/" + size + "/" + bits_ignored_str + "/similarity.out";
    }

    if ((function == "tableUpdate") && local) {
        if (binary_file) {
            file_path = "/home/vic/Documents/MultiCacheSim/tests/traces/data/(" + entries_str + ")" + state + "_" + updates_str + "_binary.out";
        }
        else {
            file_path = "/home/vic/Documents/MultiCacheSim/tests/traces/data/(" + entries_str + ")" + state + "_" + updates_str + ".out";
        }
    }
    else if ((function == "modifyData") && local) {
        if (binary_file) {
            file_path = "/home/vic/Documents/MultiCacheSim/tests/traces/data/(" + entries_str + ")" + state + "_" + method + "_" + updates_str + "_binary.out";
        }
        else {
            file_path = "/home/vic/Documents/MultiCacheSim/tests/traces/data/(" + entries_str + ")" + state + "_" + method + "_" + updates_str + ".out";
        }
    }

    return file_path;
}

std::string Cache::infile_generation(std::string function, const std::string method, const std::string suite, const std::string benchmark, const std::string size, const int entries, \
    const int bits_ignored, const int updates, const std::string state, const int binary_file)
{
    std::string file_path;

    auto updates_str = std::to_string(updates);
    auto entries_str = std::to_string(entries);
    auto bits_ignored_str = std::to_string(bits_ignored);

    if (function == "modifyData") {
        if (binary_file) {
            file_path = "/aenao-99/karyofyl/results/mcs/" + suite + "/" + benchmark + "/" + size + "/compressibility/(" + entries_str + ")" + state + "_" + updates_str + "_binary.out";
        }
        else {
            file_path = "/aenao-99/karyofyl/results/mcs/" + suite + "/" + benchmark + "/" + size + "/compressibility/(" + entries_str + ")" + state + "_" + updates_str + ".out";
        }
    }

    if ((function == "modifyData") && local) {
        if (binary_file) {
            file_path = "/home/vic/Documents/MultiCacheSim/tests/traces/data/(" + entries_str + ")" + state + "_" + updates_str + "_binary.out";
        }
        else {
            file_path = "/home/vic/Documents/MultiCacheSim/tests/traces/data/(" + entries_str + ")" + state + "_" + updates_str + ".out";
        }
        
    }

    return file_path;
}

void Cache::snapshot(const std::string cacheState)
{
    if (cacheState == "before") std::cout << "\nBefore Snapshot\n\n";
    if (cacheState == "after") std::cout << "\nAfter Snapshot\n\n";
    for (uint i=0; i<sets.size(); i++) {
        int way_count = 0;
        std::cout << "Cache Line #" << i << " \n";
        for (auto it = sets[i].begin(); it != sets[i].end(); ++it) {
            std::cout << "Way #" << way_count << ", Tag: " << std::hex << it->tag << ", Data: (";
            for (int j=0; j<64; j++) {
                if (j != 63) {
                    std::cout << std::hex << it->data[j] << " ";
                }
                else {
                    std::cout << std::hex << it->data[j] << ")";
                }
            }
            way_count++;
            std::cout << "\n";
        }
    }
    std::cout << "_________________\n\n";

}

int Cache::cacheElements()
{
    int elements = 0;

    //Iterate over the cache and track the lines currently in it
    for (uint i=0; i<sets.size(); i++) {
        for (auto it = sets[i].begin(); it != sets[i].end(); ++it) {
            elements++;
        }
    }

    return elements;
}

void Cache::checkSimilarity(std::array<int,64> lineData, int maskedBits, char rw)
{
    std::array<int,64> maskedData;

    if (maskedBits == 1) {
        for (int i=0; i<64; i++) {
            maskedData[i] = lineData[i];
            if (i % 8 == 7) {
                maskedData[i] = lineData[i] & 0xFE;
            }
        }
        if (rw == 'W') {
            occurence.insert({maskedData,0});
        }
        else if (rw == 'R') {
            occurence[maskedData]++;
        }
    }
    else if (maskedBits == 2) {
        for (int i=0; i<64; i++) {
            maskedData[i] = lineData[i];
            if (i % 8 == 7) {
                maskedData[i] = lineData[i] & 0xFC;
            }
        }
        if (rw == 'W') {
            occurence.insert({maskedData,0});
        }
        else if (rw == 'R') {
            occurence[maskedData]++;
        }
    }
    else if (maskedBits == 4) {
        for (int i=0; i<64; i++) {
            maskedData[i] = lineData[i];
            if (i % 8 == 7) {
                maskedData[i] = lineData[i] & 0xF0;
            }
        }
        if (rw == 'W') {
            occurence.insert({maskedData,0});
        }
        else if (rw == 'R') {
            occurence[maskedData]++;
        }
    }
    else if (maskedBits == 8) {
        for (int i=0; i<64; i++) {
            maskedData[i] = lineData[i];
            if (i % 8 == 7) {
                maskedData[i] = 0;
            }
        }
        if (rw == 'W') {
            occurence.insert({maskedData,0});
        }
        else if (rw == 'R') {
            occurence[maskedData]++;
        }
    }
    else if (maskedBits == 16) {
        for (int i=0; i<64; i++) {
            maskedData[i] = lineData[i];
            if (i % 8 == 6 || i % 8 == 7) {
                maskedData[i] = 0;
            }
        }
        if (rw == 'W') {
            occurence.insert({maskedData,0});
        }
        else if (rw == 'R') {
            occurence[maskedData]++;
        }
    }
    else if (maskedBits == 32) {
        for (int i=0; i<64; i++) {
            maskedData[i] = lineData[i];
            if (i % 8 == 4 || i % 8 == 5 || i % 8 == 6 || i % 8 == 7) {
                maskedData[i] = 0;
            }
        }
        if (rw == 'W') {
            occurence.insert({maskedData,0});
        }
        else if (rw == 'R') {
            occurence[maskedData]++;
        }
    }
    else {
        if (rw == 'W') {
            occurence.insert({lineData,0});
        }
        else if (rw == 'R') {
            occurence[lineData]++;
        }
    }
}

void Cache::printSimilarity(const int updates, const std::string benchmark, const std::string suite, const std::string size, const int entries, const std::string method, const int bits_ignored)
{
    std::string function = "printSimilarity";
    // State is irrelevant in this function
    string state = "after";
    std::string similarity_outfile = this->outfile_generation(function, method, suite, benchmark, size, entries, bits_ignored, updates, state, 0);
    std::ofstream similarity_trace(similarity_outfile.c_str());

    int all_reads = 0;
    int unique_lines = 0;
    // std::cout << "\nSimilarity Stats\n_________________\n\n";
    for (auto it = occurence.begin(); it != occurence.end(); ++it) {
        all_reads = all_reads + it->second;
        unique_lines++;
    }
    // std::cout << "Total reads: " << all_reads << "\n\n";
    similarity_trace << all_reads << "\n";
    similarity_trace << unique_lines << "\n";
    similarity_trace << "reads,percentage\n";
    for (auto it = occurence.begin(); it != occurence.end(); ++it) {
        double percentage = (double(it->second)/all_reads)*100;
        // std::cout << "Cache Data: ";
        // for (int i=0; i<64; i++) {
        //     similarity_trace << it->first[i] << " ";
        // }
        // similarity_trace << ",";
        similarity_trace << it->second << "," << std::fixed << std::setprecision(2) << percentage << "\n";
    }
    similarity_trace.close();
}

// Kmeans
bool Cache::tableUpdate(const int updates, const std::string benchmark, const std::string suite, const std::string size, const int entries, const std::string method, const int bits_ignored)
{
    std::string function = "tableUpdate";
    std::string state;

    std::vector<std::array<int,64>> inputData;

    //Iterate over the cache and keep the cache data as input
    for (uint i=0; i<sets.size(); i++) {
        for (auto it = sets[i].begin(); it != sets[i].end(); ++it) {
            inputData.push_back(it->data);
        }
    }

    // Test for printing the data that goes into the "inputData" vector
    // for (uint i=0; i<sets.size(); i++) {
    //     int way_count = 0;
    //     std::cout << "Cache Line #" << i << " \n";
    //     for (auto it = sets[i].begin(); it != sets[i].end(); ++it) {
    //         std::cout << "Way #" << way_count << ", Tag: " << std::hex << it->tag << ", Data: (";
    //         inputData.push_back(it->data);
    //         for (int j=0; j<64; j++) {
    //             if (j != 63) {
    //                 std::cout << std::hex << it->data[j] << " ";
    //             }
    //             else {
    //                 std::cout << std::hex << it->data[j] << ")";
    //             }
    //         }
    //         way_count++;
    //         std::cout << "\n\n";
    //     }
    // }

    int cache_elements = cacheElements();
    bool kmeans_run = false;

    if (cache_elements >= entries) {

        /* Non-binary files */
        // Before file (cache data before precompression)
        state = "before";
        std::string before_outfile = this->outfile_generation(function, method, suite, benchmark, size, entries, bits_ignored, updates, state, 0);
        std::ofstream before_trace(before_outfile.c_str());
        // Precompression table entries
        state = "table";
        std::string table_outfile = this->outfile_generation(function, method, suite, benchmark, size, entries, bits_ignored, updates, state, 0);
        std::ofstream table_trace(table_outfile.c_str());
        // Mapping between precompression table entries and cache lines
        state = "mapping";
        std::string mapping_outfile = this->outfile_generation(function, method, suite, benchmark, size, entries, bits_ignored, updates, state, 0);
        std::ofstream mapping_trace(mapping_outfile.c_str());

        /* Binary files */
        state = "before";
        std::string before_outfile_binary = this->outfile_generation(function, method, suite, benchmark, size, entries, bits_ignored, updates, state, 1);
        std::ofstream before_trace_binary(before_outfile_binary.c_str(), ios::binary);

        // cout << "Cache Elements = " << cache_elements << "\n";
        auto cluster_data = dkm::kmeans_lloyd(inputData,entries);

        if (enable_prints) std::cout << "Means:\n\n";
        int means = 0;
        for (const auto& mean : std::get<0>(cluster_data)) {
            if (enable_prints) std::cout << "#" << means << ": (";
            means++;
            for (int i=0; i<64; i++) {
                if (i != 63) {
                    table_trace << std::hex << mean[i] << " ";
                    if (enable_prints) cout << std::hex << mean[i] << " ";
                }
                else {
                    table_trace << std::hex << mean[i];
                    if (enable_prints) cout << std::hex << mean[i] << " ";
                }
                
            }
            if (enable_prints) std::cout << ")\n\n";
            table_trace << "\n";
        }

        if (enable_prints) std::cout << "\nCluster mapping:\n";
        if (enable_prints) std::cout << "\tPoint:\n";
        for (const auto& point : inputData) {
            std::stringstream value;
            std::stringstream value_file;
            value << "\t(";
            for (int i=0; i<64; i++) {
                if (i != 63) {
                    value << std::hex << point[i] << " ";
                    value_file << std::hex << point[i] << " ";
                }
                else {
                    value << std::hex << point[i];
                    value_file << std::hex << point[i];
                }
                // Binary File Writing
                // std::string binary = std::bitset<8>(point[i]).to_string();
                // before_trace_binary.write(binary.c_str(), sizeof(std::bitset<8>));
            }
            value << ")";
            if (enable_prints) std::cout << value.str() << "\n";
            before_trace << value_file.str() << "\n";
        }

        if (enable_prints) std::cout << "\n\tLabel:";
        std::stringstream labels;
        labels << "(";
        for (const auto& label : std::get<1>(cluster_data)) {
            labels << label << ",";
            mapping_trace << label << "\n";
        }
        labels << ")";
        if (enable_prints) std::cout << labels.str() << "\n";


        before_trace.close();
        table_trace.close();
        mapping_trace.close();

        before_trace_binary.close();

        kmeans_run = true;
    }
    
    return kmeans_run;

}

void Cache::modifyData(const int updates, const std::string benchmark, const std::string suite, const std::string size, const int entries, const std::string method, const int bits_ignored)
{

    std::string function = "modifyData";

    /* Non-binary files */
    // After file (cache data after precompression)
    std::string state = "after";
    std::string after_outfile = this->outfile_generation(function, method, suite, benchmark, size, entries, bits_ignored, updates, state, 0);
    std::ofstream after_trace(after_outfile.c_str());

    /* Binary files */
    state = "after";
    std::string after_outfile_binary = this->outfile_generation(function, method, suite, benchmark, size, entries, bits_ignored, updates, state, 1);
    std::ofstream after_trace_binary(after_outfile_binary.c_str(), ios::binary);

    int lines = 1;
    int lines_test = 1;

    // Read data from before file
    state = "before";
    std::string before_infile = this->infile_generation(function, method, suite, benchmark, size, entries, bits_ignored, updates, state, 0);
    std::ifstream before_trace(before_infile.c_str());

    std::vector<std::array<int,64>> beforeData;

    std::string line;

    while(getline(before_trace,line))
    {
        istringstream iss(line);
        std::array<int,64> lineData;
        int value;
        for (int i=0; i<64; i++) {
            iss >> std::hex >> value;
            lineData[i] = value;
        }
        beforeData.push_back(lineData);
        lines++;
    }

    // Read data from mapping file
    state = "mapping";
    std::string mapping_infile = this->infile_generation(function, method, suite, benchmark, size, entries, bits_ignored, updates, state, 0);
    std::ifstream mapping_trace(mapping_infile.c_str());

    std::vector<int> mappingData;

    while(getline(mapping_trace,line))
    {
        istringstream iss(line);
        int value;
        iss >> value;
        mappingData.push_back(value);
        lines_test++;
    }

    assert(lines == lines_test);

    // Read data from table file
    state = "table";
    std::string table_infile = this->infile_generation(function, method, suite, benchmark, size, entries, bits_ignored, updates, state, 0);
    std::ifstream table_trace(table_infile.c_str());

    std::vector<std::array<int,64>> tableEntries;

    while(getline(table_trace,line))
    {
        istringstream iss(line);
        std::array<int,64> entry;
        int value;
        for (int i=0; i<64; i++) {
            iss >> std::hex >> value;
            entry[i] = value;
        }
        tableEntries.push_back(entry);
    }

    // Modify data (distinguish based on method - xor or add)
    std::vector<std::array<int,64>> afterData;

    for (uint i=0; i<beforeData.size(); i++) {

        int mapped_entry = mappingData[i];
        std::array<int,64> afterData_entry;

        for (int j=0; j<64; j++) {
            if (method == "xor") {
                afterData_entry[j] = beforeData[i][j] ^ tableEntries[mapped_entry][j];
                if ((afterData_entry[j]) > 255 || (afterData_entry[j]) < -255) {
                    std::cout << "\n! The value of the modified cache data is outside of bounds (-255 - 255)! (afterData_entry) with a value of:" << afterData_entry[j] << "\n";
                    abort();
                }
            }
            else if (method == "add") {
                // FIXME: How to address when values are outside of [0,255]
                afterData_entry[j] = beforeData[i][j] - tableEntries[mapped_entry][j];
                if ((afterData_entry[j]) > 255 || (afterData_entry[j]) < -255) {
                    std::cout << "\n! The value of the modified cache data is outside of bounds (-255 - 255)! (afterData_entry) with a value of:" << afterData_entry[j] << "\n";
                    abort();
                }
            }
            else {
                std::cout << "\n! The method you provided is wrong !\n";
                abort();
            }
        }
        afterData.push_back(afterData_entry);
    }

    // Write data to after file
    for (uint i=0; i<afterData.size(); i++) {
        std::stringstream value_file;
        for (int j=0; j<64; j++) {
            if (j!=63) {
                value_file << std::hex << afterData[i][j] << " ";
                if ((afterData[i][j]) > 255 || (afterData[i][j]) < -255) {
                    std::cout << "\n! The value of the modified cache data is outside of bounds (-255 - 255)! (afterData) with a value of:" << afterData[i][j] << "\n";
                    abort();
                }
            }
            else {
                value_file << std::hex << afterData[i][j];
                if ((afterData[i][j]) > 255 || (afterData[i][j]) < -255) {
                    std::cout << "\n! The value of the modified cache data is outside of bounds (-255 - 255)! (afterData) with a value of:" << afterData[i][j] << "\n";
                    abort();
                }
            }
            // Binary File Writing
            // std::string binary = std::bitset<8>(afterData[i][j]).to_string();
            // after_trace_binary.write(binary.c_str(), sizeof(std::bitset<8>));
        }
        after_trace << value_file.str() << "\n";
    }

    after_trace.close();
}