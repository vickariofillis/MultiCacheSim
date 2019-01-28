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

#include <cassert>
#include <iostream>
#include <iomanip>
#include <iterator>
#include <fstream>

#include "misc.h"
#include "cache.h"
// Kmeans
/* Local */
// #include "/home/vic/Documents/dkm-master/include/dkm_parallel.hpp"
/* Cluster */
#include "/aenao-99/karyofyl/dkm-master/include/dkm_parallel.hpp"

bool enable_prints = 0;
bool enable_prints_file = 0;

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

std::string Cache::outfile_generation(std::string function, const std::string suite, const std::string benchmark, const std::string size, const int bits_ignored, const int updates,\
     const std::string state)
{

    std::string file_path;
    std::string type = "apps";
    std::string extra1 = "/pkgs/";
    std::string extra2 = "/test";
    std::string extra3 = "/run";

    if (suite == "parsec") {
        // cout << "\nInto suite=parsec\n";
        if (benchmark == "blackscholes" || benchmark == "bodytrack" || benchmark == "facesim" || benchmark == "ferret" || benchmark == "fluidanimate" || benchmark == "freqmine" ||
            benchmark == "raytrace" || benchmark == "swaptions" || benchmark == "vips" || benchmark == "x264" || benchmark == "test") {
            // cout << "Into apps\n";
            type = "apps";
        }
        else if (benchmark == "canneal" || benchmark == "dedup" || benchmark == "streamcluster") {
            // cout << "Into kernels\n";
            type = "kernels";
        }
        // cout << "Before extra1+extra2\n";
        extra1 = "/pkgs/";
        extra2 = "/" + benchmark;
    }
    else if (suite == "perfect") {
        // cout << "\nInto suite=perfect\n";
        // FIX-ME: possibly wrong path
        type = "";
        extra1 = "";
        extra2 = "";
        extra3 = "";
    }
    else if (suite == "phoenix") {
        // cout << "\nInto suite=phoenix\n";
        type = "";
        extra1 = "";
        extra2 = "";
        extra3 = "";
    }

    if (function == "tableUpdate") {
        auto updates_str = std::to_string(updates);
        file_path = "/aenao-99/karyofyl/results/pin/pinatrace/" + suite + "/" + benchmark + "/" + size + extra1 + type + extra2 + extra3 + "/" + state + updates_str + ".out";
    }

    return file_path;
}

void Cache::snapshot()
{
    std::cout << "\nSnapshot\n\n";
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
            std::cout << "\n\n";
        }
    }
    std::cout << "\n";

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

void Cache::printSimilarity(const int bits_ignored, const std::string benchmark)
{

    auto str_int = std::to_string(bits_ignored);
    std::string file_path = "/aenao-99/karyofyl/results/mcs/parsec/" + benchmark + "/small/" + str_int + "/similarity.out";
    std::ofstream trace(file_path.c_str());

    int all_reads = 0;
    int unique_lines = 0;
    // std::cout << "\nSimilarity Stats\n_________________\n\n";
    for (auto it = occurence.begin(); it != occurence.end(); ++it) {
        all_reads = all_reads + it->second;
        unique_lines++;
    }
    // std::cout << "Total reads: " << all_reads << "\n\n";
    trace << all_reads << "\n";
    trace << unique_lines << "\n";
    trace << "reads,percentage\n";
    for (auto it = occurence.begin(); it != occurence.end(); ++it) {
        double percentage = (double(it->second)/all_reads)*100;
        // std::cout << "Cache Data: ";
        // for (int i=0; i<64; i++) {
        //     trace << it->first[i] << " ";
        // }
        // trace << ",";
        trace << it->second << "," << std::fixed << std::setprecision(2) << percentage << "\n";
    }
    trace.close();
}

// Kmeans
void Cache::tableUpdate(const int updates, const std::string benchmark, const std::string suite, const std::string size, const int entries, const std::string method, const int bits_ignored)
{
    std::string function = "tableUpdate";

    std::vector<std::array<int,64>> inputData;

    // Test before data
    std::string state = "before_test";
    std::string before_test_outfile = this->outfile_generation(function, suite, benchmark, size, bits_ignored, updates, state);
    std::ofstream before_test_trace(before_test_outfile.c_str());

    //Iterate over the cache and keep the cache data as input
    for (uint i=0; i<sets.size(); i++) {
        for (auto it = sets[i].begin(); it != sets[i].end(); ++it) {
            inputData.push_back(it->data);
            for (int j=0; j<64; j++) {
                if (j != 63) {
                    before_test_trace << std::hex << it->data[j] << " ";
                }
                else {
                    before_test_trace << std::hex << it->data[j] << "\n";
                }
            }
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

    // Before file (cache data before precompression)
    state = "before";
    std::string before_outfile = this->outfile_generation(function, suite, benchmark, size, bits_ignored, updates, state);
    std::ofstream before_trace(before_outfile.c_str());
    // After file (cache data after precompression)
    state = "after";
    std::string after_outfile = this->outfile_generation(function, suite, benchmark, size, bits_ignored, updates, state);
    std::ofstream after_trace(after_outfile.c_str());
    // Precompression table entries
    state = "table";
    std::string table_outfile = this->outfile_generation(function, suite, benchmark, size, bits_ignored, updates, state);
    std::ofstream table_trace(table_outfile.c_str());

    auto cluster_data = dkm::kmeans_lloyd(inputData,entries);

    //FIXME: print the before files (includes the cache lines with the mapping + file of Precompression table entries)
    std::cout << "Means:\n\n";
    int means = 0;
    for (const auto& mean : std::get<0>(cluster_data)) {
        std::cout << "#" << means << ": (";
        means++;
        for (int i=0; i<64; i++) {
            if (i != 63) {
                std::cout << mean[i] << " ";
                table_trace << mean[i] << " ";
            }
            else {
                std::cout << mean[i];
                table_trace << mean[i];
            }
            
        }
        std::cout << ")\n\n";
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
        }
        value << ")";
        if (enable_prints) std::cout << value.str() << "\n";
        before_trace << value_file.str() << "\n";
    }
    if (enable_prints) std::cout << "\n\tLabel:";
    std::stringstream labels;
    std::stringstream labels_file;
    labels << "(";
    for (const auto& label : std::get<1>(cluster_data)) {
        if (enable_prints) labels << label << ",";
        // labels_file << label;
        // table_trace << labels_file.str() << "\n";
        table_trace << label << "\n";
    }
    labels << ")";
    std::cout << labels.str() << "\n";

    before_test_trace.close();
    before_trace.close();
    after_trace.close();
    table_trace.close();
}

//FIXME: Have a function that reads the before file and produces the after file