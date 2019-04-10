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
#include <typeinfo>

#include "misc.h"
#include "cache.h"

// Kmeans
/* Cluster */
#include "/aenao-99/karyofyl/dkm-master/include/dkm_parallel.hpp"
/* Local */
// #include "/home/vic/Documents/dkm-master/include/dkm_parallel.hpp"

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

   sets[set].emplace_back(tag, state, -1, data, data);
}

// Updating cache data in case of a hit
void Cache::updateData(uint64_t set, uint64_t tag, std::array<int,64> data, std::string method, std::string hit_update)
{
    for (auto it = sets[set].begin(); it != sets[set].end(); ++it) {
        if (it->tag == tag) {
            if (hit_update == "n") {
                it->mapping = -1;
            }
            for (int j=0; j<64; j++) {
                if (hit_update == "n") {
                    it->data[j] = data[j];
                    it->datax[j] = data[j];
                }
                else {
                    // FIXME: Add other precompression methods
                    if (method == "xor") {
                        it->data[j] = data[j];
                        it->datax[j] = it->data[j] ^ clusterData[it->mapping][j];
                        // xorData(set);    //FIXME
                    }
                }
            }
        }
    }
}

// Kmeans
void Cache::precompress(std::string method, int entries)
{
    int cacheLines = 0;
    std::vector<std::array<int,64>> inputData;

    //Iterate over the cache and keep the cache data as input
    for (uint i=0; i<sets.size(); i++) {
        for (auto it = sets[i].begin(); it != sets[i].end(); ++it) {
            inputData.push_back(it->data);
            cacheLines++;
        }
    }

    std::tuple<std::vector<std::array<int, 64>>, std::vector<uint32_t>> dkmData;
    if (cacheLines < entries) {
        dkmData = dkm::kmeans_lloyd_parallel(inputData,cacheLines);
    }
    else {
        dkmData = dkm::kmeans_lloyd_parallel(inputData,entries);
    }

    // Delete centroids vector contents before updating it
    clusterData.clear();

    // Updating clusterData (centroids)
    std::array<int,64> tempCentroid = {0};
    for (const auto& mean : std::get<0>(dkmData)) {
        for (int i=0; i<64; i++) {
            tempCentroid[i] = mean[i];
        }
        clusterData.push_back(tempCentroid);
    }

    // Updating mappings
    std::vector<uint32_t> labels;
    int label_counter = 0;      // used for debugging (must be the same as the number of cache lines)
    // Saving labels into a vector
    for (const auto& label : std::get<1>(dkmData)) {
        labels.push_back(label);
        label_counter++;
    }
    assert(cacheLines == label_counter);
    int lines_counter = 0;      // used for assigning the labels to each cache line
    for (uint i=0; i<sets.size(); i++) {
        for (auto it = sets[i].begin(); it != sets[i].end(); ++it) {
            it->mapping = labels[lines_counter];
            lines_counter++;
        }
    }

    // Update datax
    for (uint i=0; i<sets.size(); i++) {
        for (auto it = sets[i].begin(); it != sets[i].end(); ++it) {
            for (int j=0; j<64; j++) {
                // FIXME: Add other precompression methods
                if (method == "xor") {
                    it->datax[j] = it->data[j] ^ clusterData[it->mapping][j];
                    if ((it->datax[j]) > 255 || (it->datax[j]) < -255) {
                        std::cout << "\n! The value of the modified cache data is outside of bounds (-255 - 255)! with a value of:" << it->datax[j] << "\n";
                        abort();
                    }
                    // xorData(i);  //FIXME
                }
            }
        }
    }
}

// FIXME: Xor precompression function
// void Cache::xorData(uint64_t set)
// {
//     // Update datax for current line
//     auto it = sets[set];
//     for (int j=0; j<64; j++) {
//         it->datax[j] = it->data[j] ^ clusterData[it->mapping][j];
//         if ((it->datax[j]) > 255 || (it->datax[j]) < -255) {
//             std::cout << "\n! The value of the modified cache data is outside of bounds (-255 - 255)! with a value of:" << it->datax[j] << "\n";
//             abort();
//         }
//     }
// }

void Cache::snapshot(int cache_num)
{
    std::cout << "Cache: " << cache_num << "\n\n";
    for (uint i=0; i<sets.size(); i++) {
        int way_count = 0;
        std::cout << "Cache Line #" << i << " \n";
        for (auto it = sets[i].begin(); it != sets[i].end(); ++it) {
            std::cout << "Way #" << way_count << ", Tag: " << std::hex << it->tag << ", Mapping: " << std::dec << it->mapping << "\nData: (";
            for (int j=0; j<64; j++) {
                if (j != 63) {
                    std::cout << std::hex << it->data[j] << " ";
                }
                else {
                    std::cout << std::hex << it->data[j] << ")";
                }
            }
            std::cout << "\nDatax: (";
            for (int j=0; j<64; j++) {
                if (j != 63) {
                    std::cout << std::hex << it->datax[j] << " ";
                }
                else {
                    std::cout << std::hex << it->datax[j] << ")";
                }
            }
            way_count++;
            std::cout << "\n";
        }
    }

    std::cout << "\nPrecompression Table Entries\n\n";
    for (uint i=0; i<clusterData.size(); i++) {
        std::cout << "Entry #" << i << "\n";
        std::cout << "(";
        for (int j=0; j<64; j++) {
            if (j != 63) {
                std::cout << std::hex << clusterData[i][j] << " ";
            }
            else {
                std::cout << std::hex << clusterData[i][j] << ")";
            }
        }
        std::cout << "\n";
    }

    std::cout << "________________________________________________________________________________________________\n\n";

}
