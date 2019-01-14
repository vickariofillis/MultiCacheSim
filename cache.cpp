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
#include "kmeans.h"

// std::ofstream trace("/aenao-99/karyofyl/results/mcs/parsec/" + benchmark + "/small" + bits_ignored + "/similarity.out");

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

void Cache::snapshot()
{
    int way_count;
    // int data_count;

    for (uint i=0; i<sets.size(); i++) {
        way_count = 0;
        std::cout << "Cache Line #" << i << " \n";
        for (auto it = sets[i].begin(); it != sets[i].end(); ++it) {
            // data_count = 0;
            std::cout << "Way #" << way_count << ", Tag: " << std::hex << it->tag << ", Data: ";
            for (int j=0; j<64; j++) {
                std::cout << std::hex << it->data[j] << " ";
                // data_count++;
            }
            way_count++;
            std::cout << "\n\n";
            // std::cout << "Data count: " << std::dec << data_count << "\n";
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

void Cache::printSimilarity(int bits_ignored, std::string benchmark)
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

void Cache::tableUpdate(int entries, std::string method, int bits_ignored)
{
    int line_cnt = 0;
    std::vector<std::array<int,64>> inputData;

    //Iterate over the cache and keep the cache data as input
    for (uint i=0; i<sets.size(); i++) {
        line_cnt++;
        for (auto it = sets[i].begin(); it != sets[i].end(); ++it) {
            inputData.push_back(it->data);
        }
    }

    Entries cluster(line_cnt, 64, entries, 100);
    // max_iterations = 100
    cluster.clustering(*inputData);
}