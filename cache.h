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

#pragma once

#include <vector>
#include <array>
#include <unordered_map>
#include <deque>

#include "misc.h"

// File generation for printing most frequent entries
std::string frequent_entries_outfile_generation(const std::string machine, const std::string suite, const std::string benchmark, const std::string size, const int entries, const int data_type, \
    const int bytes_ignored);

// Represents a single cache
class Cache {
public:
    // num_lines is total line capacity, assoc is the number of ways (locations)
    // a single line can be placed
    Cache(unsigned int num_lines, unsigned int assoc);
    // Returns the state of specified line, or Invalid if not found
    CacheState findTag(uint64_t set, uint64_t tag) const; 
    void changeState(uint64_t set, uint64_t tag, CacheState state);
    // Line must exist in the cache
    void updateLRU(uint64_t set, uint64_t tag);
    // Returns true if writeback necessary, and the tag of the line if true
    bool checkWriteback(uint64_t set, uint64_t& tag) const;
    // Line should not already exist in cache. Will remove the LRU line in set
    // if there is not enough space, so checkWriteback should be called before this
    void insertLine(uint64_t set, uint64_t tag, CacheState state, std::array<int,64> data, std::string precomp_method, std::string precomp_update_method, std::string comp_method, \
        int entries, std::string infinite_freq, std::string ignore_i_bytes, int data_type, int bytes_ignored, int sim_threshold);
    // Updating cache on a write hit
    void updateData(uint64_t set, uint64_t tag, std::array<int,64> data, std::string precomp_method, std::string precomp_update_method, std::string comp_method, int entries, std::string infinite_freq, \
        std::string hit_update, std::string ignore_i_bytes, int data_type, int bytes_ignored, int sim_threshold);
    // Returns the way number of the specified cache line
    uint getWay(uint64_t set, uint64_t tag) const;
    // Update frequency table
    void updateFrequencyTable(std::array<int,64> data, int entries, std::string infinite_freq, int data_type, int bytes_ignored);
    // Updating precompression table
    void updatePrecompressTable(std::string machine, std::string suite, std::string benchmark, std::string size, int entries, std::string precomp_method, std::string precomp_update_method, \
        std::string infinite_freq, std::string ignore_i_bytes, int data_type, int bytes_ignored, int sim_threshold);
    // Computing data after precompression is applied (for a single cache line)
    void precompressDatax(std::string precomp_method, uint64_t set, uint64_t tag, int data_type, int bytes_ignored, std::string ignore_i_bytes);
    // Computing data after precompression is applied for the entire cache
    void precompressCache(std::string precomp_method, int data_type, int bytes_ignored, std::string ignore_i_bytes);
    // Precompression technique (xor)
    // void xorData(uint64_t set);   //FIXME
    // Finding a similar Precompression Entry (for mapping)
    int findPrecompressionEntry(std::array<int,64> data, std::string precomp_update_method, int data_type, int bytes_ignored, int sim_threshold);
    // Compression stats 
    std::tuple<int, int, double, double> compressionStats(int cache_line_num, int assoc, int cache_num, std::string comp_method);
    // Printing cache data 
    void snapshot(int cache_num);
private:
    std::vector<std::deque<CacheLine>> sets;
    unsigned int maxSetSize;
    std::vector<std::array<int,64>> clusterData;
    std::vector<std::tuple<std::array<int,64>,int>> infiniteFrequentLines;
    std::deque<std::array<int,64>> frequentLines;
};

