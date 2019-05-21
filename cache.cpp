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
#include <fstream>
#include <iostream>
#include <iomanip>
#include <iterator>
#include <numeric>
#include <typeinfo>

#include "misc.h"
#include "cache.h"
#include "compression.h"

// Kmeans
/* Cluster */
#include "/aenao-99/karyofyl/dkm-master/include/dkm_parallel.hpp"
/* Local */
// #include "/home/vic/Documents/dkm-master/include/dkm_parallel.hpp"

// File generation for printing most frequent entries
std::string frequent_entries_outfile_generation(const std::string machine, const std::string suite, const std::string benchmark, const std::string size, const int entries, const int data_type, \
    const int bytes_ignored)
{
    std::string file_path;

    if (machine == "cluster") {
        file_path = "/aenao-99/karyofyl/results/mcs/" + suite + "/" + benchmark + "/" + size + "/compressibility/finite_frequent_entries_" + std::to_string(entries) + ".out";
    }
    else if (machine == "local") {
        file_path = "/home/vic/Documents/MultiCacheSim/tests/traces/" + suite + "/" + benchmark + "/" + size + "_frequent_entries_" + std::to_string(entries) + ".out";
    }
    

    return file_path;
}

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
void Cache::insertLine(uint64_t set, uint64_t tag, CacheState state, std::array<int,64> data, std::string precomp_method, std::string precomp_update_method, std::string comp_method, int entries, \
    std::string infinite_freq, std::string ignore_i_bytes, int data_type, int bytes_ignored, int sim_threshold)
{
    if (sets[set].size() == maxSetSize) {
      sets[set].pop_front();
    }

    // Generic cache line insertion
    // sets[set].emplace_back(tag, state, -1, data, data);

    if (precomp_update_method == "kmeans") {
        sets[set].emplace_back(tag, state, -1, data, data);
    }
    else if (precomp_update_method == "frequency") {

        // Update frequency table
        updateFrequencyTable(data, entries, infinite_freq, data_type, bytes_ignored);

        // debug
        // std::cout << "findPrecompressionEntry <- insertLine\n";

        // Check if a precompression table entry is similar to the inserted data
        int similarEntry = findPrecompressionEntry(data, precomp_update_method, data_type, bytes_ignored, sim_threshold);
        // std::cout << "similarEntry = " << similarEntry << "\n"; // debug

        if (similarEntry > -1) {
            //call precompressDatax
            sets[set].emplace_back(tag, state, similarEntry, data, data);
            // debug
            // std::cout << "precompressDatax <- insertLine\n";
            precompressDatax(precomp_method, set, tag, data_type, bytes_ignored, ignore_i_bytes);
        }
        else {
            sets[set].emplace_back(tag, state, -1, data, data);
        }
    }

    // FIXME: Call function responsible for getting bdi stats (input is cache line data) - ONLY IF I CARE ABOUT THE NEWLY INSERTED LINE

}

// Updating cache data in case of a hit
void Cache::updateData(uint64_t set, uint64_t tag, std::array<int,64> data, std::string precomp_method, std::string precomp_update_method, std::string comp_method, int entries, \
    std::string infinite_freq, std::string hit_update, std::string ignore_i_bytes, int data_type, int bytes_ignored, int sim_threshold)
{
    for (auto it = sets[set].begin(); it != sets[set].end(); ++it) {
        if (it->tag == tag) {
            if (hit_update == "n") {
                it->mapping = -1;
                for (int j=0; j<64; j++) {
                    it->data[j] = data[j];
                    it->datax[j] = data[j];
                }
            }
            if (precomp_update_method == "kmeans") {
                if (hit_update == "y") {
                    for (int j=0; j<64; j++) {
                        // FIXME: Add other precompression methods
                        if (precomp_method == "xor") {
                            it->data[j] = data[j];
                            it->datax[j] = it->data[j] ^ clusterData[it->mapping][j];
                            // xorData(set);    //FIXME
                        }
                    }
                }
            }
            else if (precomp_update_method == "frequency") {

                // Update frequency table
                updateFrequencyTable(data, entries, infinite_freq, data_type, bytes_ignored);

                if (hit_update == "y") {
                    // Update data
                    for (int j=0; j<64; j++) {
                        it->data[j] = data[j];
                    }
                    /* Update datax */

                    // debug
                    // std::cout << "findPrecompressionEntry <- updateData\n";

                    // Check if a precompression table entry is similar to the updated data
                    int similarEntry = findPrecompressionEntry(data, precomp_update_method, data_type, bytes_ignored, sim_threshold);
                    // std::cout << "similarEntry = " << std::dec << similarEntry << "\n"; // debug

                    if (similarEntry > -1) {
                        // debug
                        // std::cout << "precompressDatax <- updateData\n";
                        precompressDatax(precomp_update_method, set, tag, data_type, bytes_ignored, ignore_i_bytes);
                    }
                    else {
                        // If there is no similar entry, datax is the same as data
                        for (int j=0; j<64; j++) {
                            it->datax[j] = data[j];
                        }
                        // If there is no similar entry, update the mapping indicator to reflect that
                        it->mapping = -1;
                    }
                }
            }    
        }
    }

    // FIXME: Call function responsible for getting bdi stats (input is cache line data) - ONLY IF I CARE ABOUT UPDATED DATA
    // Need to save the correct cache line data, before giving it as input
}

// Returns the way number of the specified cache line
uint Cache::getWay(uint64_t set, uint64_t tag) const
{
    int way_count = 0;
    for (auto it = sets[set].cbegin(); it != sets[set].cend(); ++it) {
        if (it->tag == tag) {
            return way_count;
        }
        way_count++;
    }
}

void Cache::updateFrequencyTable(std::array<int,64> data, int entries, std::string infinite_freq, int data_type, int bytes_ignored)
{
    // Update frequency table

    // Infinite frequency table
    if (infinite_freq == "y") {
        // Frequency table is empty
        if (infiniteFrequentLines.size() == 0) {
            infiniteFrequentLines.emplace_back(std::make_tuple(data, 1));
        }
        // Frequency table is not empty
        else {
            // If the new line exists in the frequency table
            // exists: whether the new line exists in the frequency table
            bool exists = false;
            for (uint i=0; i<infiniteFrequentLines.size(); i++) {
                // similar: whether the new line is the same as the current one we are comparing it with (in the frequency table)
                bool similar = true;
                for (int j=0; j<64; j++) {
                    if (std::get<0>(infiniteFrequentLines[i])[j] != data[j]) {
                        similar = false;
                    }
                }
                if (similar) {
                    std::get<1>(infiniteFrequentLines[i])++;
                    exists = true;
                }
            }
            // If the new line does not exist in the frequency table
            if (!exists) {
                infiniteFrequentLines.emplace_back(std::make_tuple(data, 1));
            }
        }
    }
    else if (infinite_freq == "n") {

        // Flag for ignoring LSBytes when checking for similarity
        int ignore_flag = data_type;

        // debug
        // int frequency_entry = 1; 
        // std::cout << "\nBefore\n\nFrequency Table with size of: " << std::dec << frequentLines.size() << "\n";
        // for (uint i=0; i<frequentLines.size(); i++) {
        //     std::cout << "Entry = " << frequency_entry << "\n";
        //     std::cout << "Data: (";
        //     for (int j=0; j<64; j++) {
        //         if (j != 63) {
        //             std::cout << std::hex << frequentLines[i][j] << " ";
        //         }
        //         else {
        //             std::cout << std::hex << frequentLines[i][j] << ")";
        //         }
        //     }
        //     // std::cout << " - Similar = " << similar << "\n";
        //     std::cout << "\n";
        //     frequency_entry++;
        // }
        // std::cout << "\n";
        // debug

        // Checking if the entry already exists
        // found: whether the line was found in the frequency table
        bool found = false;
        
        for (uint i=0; i<frequentLines.size(); i++) {
            bool similar = true;
            for (int j=0; j<64; j++) {
                // Takes data_type and bytes_ignored into account when checking for similarity
                if (ignore_flag > bytes_ignored){
                    if (frequentLines[i][j] != data[j]) {
                        similar = false;
                    }
                }
                ignore_flag--;
                if (ignore_flag == 0) {
                    ignore_flag = data_type;
                }
            }
            if (similar) {
                frequentLines.erase(frequentLines.begin() + i);
                frequentLines.push_back(data);
                found = true;
            }
        }
        
        // Entry does not already exist
        if (!found) {
            // Frequency table is smaller than the precompression table
            if (frequentLines.size() >= entries) {
                frequentLines.pop_front();
            }
            frequentLines.push_back(data);
        }

        // debug
        // std::cout << "\n\nFound = " << found << "\n";
        // frequency_entry = 1; 
        // std::cout << "\nAfter\n\nFrequency Table with size of: " << std::dec << frequentLines.size() << "\n";
        // for (uint i=0; i<frequentLines.size(); i++) {
        //     std::cout << "Entry = " << frequency_entry << "\n";
        //     std::cout << "Data: (";
        //     for (int j=0; j<64; j++) {
        //         if (j != 63) {
        //             std::cout << std::hex << frequentLines[i][j] << " ";
        //         }
        //         else {
        //             std::cout << std::hex << frequentLines[i][j] << ")";
        //         }
        //     }
        //     // std::cout << " - Similar = " << similar << "\n";
        //     std::cout << "\n";
        //     frequency_entry++;
        // }
        // std::cout << "\n";
        // std::cout << "------------------------------------------------------------------------------------------------\n";
        // debug
        
    }
    

    // debug
    // std::cout << "\nFrequency Table with size of: " << std::dec << infiniteFrequentLines.size() << "\n";
    // for (uint i=0; i<infiniteFrequentLines.size(); i++) {
    //     for (int j=0; j<64; j++) {
    //         if (j != 63) {
    //             std::cout << std::hex << std::get<0>(infiniteFrequentLines[i])[j] << " ";
    //         }
    //         else {
    //             std::cout << std::hex << std::get<0>(infiniteFrequentLines[i])[j] << ")";
    //         }
    //     }
    //     std::cout << " / freq = " << std::dec << std::get<1>(infiniteFrequentLines[i]) << "\n"; // debug
    // }
    // std::cout << "\n";
}

// Updating precompression table
void Cache::updatePrecompressTable(std::string machine, std::string suite, std::string benchmark, std::string size, int entries, std::string precomp_method, std::string precomp_update_method, \
    std::string infinite_freq, std::string ignore_i_bytes, int data_type, int bytes_ignored, int sim_threshold)
{

    // Different ways for filling the precompression table

    // K-means
    if (precomp_update_method == "kmeans") {

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

        // debug
        // std::cout << "precompressCache <- updatePrecompressTable\n";    // debug
        // Compute precompressed data (datax) for entire cache
        precompressCache(precomp_method, data_type, bytes_ignored, ignore_i_bytes);

    }

    // Precompression table is a frequency table
    else if (precomp_update_method == "frequency") {

        if (infinite_freq == "y") {
            // Find the n = entries most frequent cache lines
            // Sort frequency table
            sort(infiniteFrequentLines.begin(),infiniteFrequentLines.end(),[](std::tuple<std::array<int,64>,int> &a, std::tuple<std::array<int,64>,int>& b) { return std::get<1>(a) > std::get<1>(b); });

            // debug
            // std::cout << "infiniteFrequentLines size = " << infiniteFrequentLines.size() << "\n";
            // std::cout << "clusterData size = " << clusterData.size() << "\n";
            // std::cout << "entries = " << entries << "\n";

            // Delete centroids vector contents before updating it
            clusterData.clear();

            // end_point is used to make sure that the Precompression table is not filled with frequent lines that do not exist
            int end_point = entries;

            if (infiniteFrequentLines.size() < entries) {
                end_point = infiniteFrequentLines.size();
            }

            // Generating filepath for printing most frequent entries
            std::string frequent_entries_outfile = frequent_entries_outfile_generation(machine, suite, benchmark, size, entries, data_type, bytes_ignored);
            std::ofstream frequent_entries_stream(frequent_entries_outfile.c_str(), std::ofstream::out | std::ofstream::app);

            // Fill Precompression table with the most frequent cache lines
            for (int i=0; i<end_point; i++) {
                std::array<int,64> tempCentroid;
                frequent_entries_stream << std::dec << i << " ";
                for (int j=0; j<64; j++) {
                    tempCentroid[j] = std::get<0>(infiniteFrequentLines[i])[j];
                    if (j != 63) {
                        frequent_entries_stream << std::dec << std::get<0>(infiniteFrequentLines[i])[j] << " ";
                    }
                    else {
                        frequent_entries_stream << std::dec << std::get<0>(infiniteFrequentLines[i])[j] << " " << std::dec << std::get<1>(infiniteFrequentLines[i]) << "\n";
                    }
                }
                clusterData.push_back(tempCentroid);
            }


        }
        else if (infinite_freq == "n") {
            // Frequency table doubles as the precompression table

            // Delete centroids vector contents before updating it
            clusterData.clear();

            // Generating filepath for printing most frequent entries
            std::string frequent_entries_outfile = frequent_entries_outfile_generation(machine, suite, benchmark, size, entries, data_type, bytes_ignored);
            std::ofstream frequent_entries_stream(frequent_entries_outfile.c_str(), std::ofstream::out | std::ofstream::app);

            // Fill Precompression table with the most frequent cache lines
            for (int i=0; i<entries; i++) {
                std::array<int,64> tempCentroid;
                for (int j=0; j<64; j++) {
                    tempCentroid[j] = frequentLines[i][j];
                    if (j != 63) {
                        frequent_entries_stream << std::dec << frequentLines[i][j] << " ";
                    }
                    else {
                        frequent_entries_stream << std::dec << frequentLines[i][j] << "\n";
                    }
                }
                clusterData.push_back(tempCentroid);
            }
        }

        // Updating mappings
        for (uint i=0; i<sets.size(); i++) {
            for (auto it = sets[i].begin(); it != sets[i].end(); ++it) {
                std::array<int,64> tempData;
                for (int j=0; j<64; j++) {
                    tempData[j] = it->data[j];
                }
                // debug
                // std::cout << "findPrecompressionEntry <- updatePrecompressTable\n"; // debug
                it->mapping = findPrecompressionEntry(tempData, precomp_update_method, data_type, bytes_ignored, sim_threshold);
                // std::cout << "it->mapping = " << it->mapping << "\n";   // debug
            }
        }

        // debug
        // std::cout << "precompressCache <- updatePrecompressTable\n";    // debug
        // Compute precompressed data (datax) for entire cache
        precompressCache(precomp_method, data_type, bytes_ignored, ignore_i_bytes);

    }
}

// Computing datax for a cache line
void Cache::precompressDatax(std::string precomp_method, uint64_t set, uint64_t tag, int data_type, int bytes_ignored, std::string ignore_i_bytes)
{
    // Flag for ignoring bytes in case ignore_i_bytes is true
    int ignore_flag = data_type;

    // Update datax on a specific cache line
    for (auto it = sets[set].begin(); it != sets[set].end(); ++it) {
        if (it->tag == tag) {
            for (int j=0; j<64; j++) {
                if (ignore_i_bytes == "y" && (ignore_flag > bytes_ignored)) {
                    if (precomp_method == "xor") {
                        it->datax[j] = it->data[j] ^ clusterData[it->mapping][j];
                        // xorData(set);    //FIXME
                    }
                }
                ignore_flag--;
                if (ignore_flag == 0) {
                    ignore_flag = data_type;
                }
                else if (ignore_i_bytes == "n") {
                    if (precomp_method == "xor") {
                        it->datax[j] = it->data[j] ^ clusterData[it->mapping][j];
                        // xorData(set);    //FIXME
                    }
                }
            }
        }
    }
}

// Computing datax for the entire cache
void Cache::precompressCache(std::string precomp_method, int data_type, int bytes_ignored, std::string ignore_i_bytes)
{
    // Flag for ignoring bytes in case ignore_i_bytes is true
    int ignore_flag = data_type;

    // std::cout << "precompressCache - precomp_method = " << precomp_method << "\n";  // debug
    // Update datax on the entire cache
    for (uint i=0; i<sets.size(); i++) {
        for (auto it = sets[i].begin(); it != sets[i].end(); ++it) {
            for (int j=0; j<64; j++) {
                if (ignore_i_bytes == "y" && (ignore_flag > bytes_ignored)) {
                    // FIXME: Add other precompression methods
                    if (precomp_method == "xor") {
                        if (it->mapping > -1) it->datax[j] = it->data[j] ^ clusterData[it->mapping][j];
                        // std::cout << "it->datax[" << std::dec << j << "] (" << std::hex << it->datax[j] << ") = it->data[" << std::dec << j << "] (" << std::hex << it->data[j] << ") ^ clusterData[" \
                        << std::dec << it->mapping << "][" << j << "] (" << std::hex << clusterData[it->mapping][j] << ")\n";   // debug
                        if ((it->datax[j]) > 255 || (it->datax[j]) < -255) {
                            // std::cout << "it->datax[" << std::dec << j << "] (" << std::hex << it->datax[j] << ") = it->data[" << std::dec << j << "] (" << std::hex << it->data[j] << ") ^ clusterData[" \
                            //     << std::dec << it->mapping << "][" << j << "] (" << std::hex << clusterData[it->mapping][j] << ")\n";   // debug
                            std::cout << "\n! The value of the modified cache data is outside of bounds (-255 - 255)! with a value of:" << it->datax[j] << "\n";
                            abort();
                        }
                        // xorData(i);  //FIXME
                    }
                }
                ignore_flag--;
                if (ignore_flag == 0) {
                    ignore_flag = data_type;
                }
                else if (ignore_i_bytes == "n") {
                    if (precomp_method == "xor") {
                        if (it->mapping > -1) it->datax[j] = it->data[j] ^ clusterData[it->mapping][j];
                        // std::cout << "it->datax[" << std::dec << j << "] (" << std::hex << it->datax[j] << ") = it->data[" << std::dec << j << "] (" << std::hex << it->data[j] << ") ^ clusterData[" \
                        << std::dec << it->mapping << "][" << j << "] (" << std::hex << clusterData[it->mapping][j] << ")\n";   // debug
                        if ((it->datax[j]) > 255 || (it->datax[j]) < -255) {
                            // std::cout << "it->datax[" << std::dec << j << "] (" << std::hex << it->datax[j] << ") = it->data[" << std::dec << j << "] (" << std::hex << it->data[j] << ") ^ clusterData[" \
                            //     << std::dec << it->mapping << "][" << j << "] (" << std::hex << clusterData[it->mapping][j] << ")\n";   // debug
                            std::cout << "\n! The value of the modified cache data is outside of bounds (-255 - 255)! with a value of:" << it->datax[j] << "\n";
                            abort();
                        }
                        // xorData(i);  //FIXME
                    }
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

// Finding a similar Precompression Entry (for mapping)
int Cache::findPrecompressionEntry(std::array<int,64> data, std::string precomp_update_method, int data_type, int bytes_ignored, int sim_threshold)
{
    // FIXME: precomp_update_method is not used for now
    // Finding the most similar precompression entry is only used for "frequency"
    // If other techniques use it, then an if statement will need to be added

    int weight = data_type;
    int sum_weight[clusterData.size()];
    // bool data_printed = false;  // debug

    // debug
    // std::cout << "Data = (";   // debug
    // for (int j=0; j<64; j++) {
    //     if (!data_printed) {
    //         if (j != 63) {
    //             std::cout << std::hex << data[j] << " ";
    //         }
    //         else {
    //             std::cout << std::hex << data[j] << ")\n";
    //         }
    //     }
    // }

    // std::cout << "bytes_ignored = " << bytes_ignored << "\n";   // debug

    for (uint i=0; i<clusterData.size(); i++) {
        sum_weight[i] = 0;
        weight = data_type;
        for (int j=0; j<64; j++) {
            // std::cout << "clusterData[" << std::dec << i << "][" << j << "] = " << std::hex << clusterData[i][j] << "  -  data[" << std::dec << j << "] = " << std::hex << data[j] << "\n";  // debug
            if (clusterData[i][j] == data[j]) {
                // std::cout << "weight (" << std::dec << weight << ") vs bytes_ignored (" << std::dec << bytes_ignored << ")" << "\n";    // debug
                if (weight > bytes_ignored) {
                    // std::cout << "before sum_weight[" << std::dec << i << "] = " << sum_weight[i] << "\n";  // debug
                    // std::cout << "sum_weight[" << std::dec << i << "] = " << sum_weight[i] << " + " << weight << "\n";  // debug
                    sum_weight[i] = sum_weight[i] + weight;
                }
            }
            weight--;
            if (weight == 0) {
                weight = data_type;
            }
        }
    }

    int similarEntry;

    if (clusterData.size() < 1) {
        similarEntry = -1;
    }
    else {

        similarEntry = 0;
        int max = sum_weight[0];

        // debug
        // std::cout << "similarity[0] = " << std::dec << sum_weight[0] << "\n";

        for (uint i=1; i<clusterData.size(); i++) {
            // debug
            // std::cout << "similarity[" << i << "] = " << std::dec << sum_weight[i] << "\n";
            if (sum_weight[i] > max) {
                max = sum_weight[i];
                similarEntry = i;
            }
        }

        // Cutoff point (return -1 if there is no similarity or it's lower than the threshold)
        if (max == 0 || max < sim_threshold) {
            similarEntry = -1;
        }
    }

    // std::cout << "returns similarEntry = " << std::dec << similarEntry << "\n"; // debug
    return similarEntry;
}

std::tuple<int, int, double, double> Cache::compressionStats(int cache_line_num, int assoc, int cache_num, std::string comp_method)
{
    std::vector<int> beforeCompressedSpace;
    std::vector<int> afterCompressedSpace;

    // Stats for cache utilization
    float cacheUtil;
    float wayUtil;
    int actual_cache_lines = 0;
    // Stats for way utilization
    int actual_way_cnt = 0;
    int max_way_cnt = 0;

    // std::cout << "\nCompression Stats\n\nCache: " << cache_num << "\n\n";   // debug

    if (comp_method == "bdi") {
        // Iterate over cache
        for (uint i=0; i<sets.size(); i++) {
            int way_count = 0;
            // std::cout << "Cache Line #" << std::dec << i << " \n";  // debug
            for (auto it = sets[i].begin(); it != sets[i].end(); ++it) {
                // std::cout << "Way #" << std::dec << way_count << "\n";  // debug
                std::array<int,64> tempData;
                std::array<int,64> tempDatax;
                for (int j=0; j<64; j++) {
                    tempData[j] = it->data[j];
                    tempDatax[j] = it->datax[j];
                }
                beforeCompressedSpace.push_back(bdi(tempData));
                afterCompressedSpace.push_back(bdi(tempDatax));
                if (sets[i].size() >= 1) {
                    actual_way_cnt = actual_way_cnt + sets[i].size();
                    max_way_cnt = max_way_cnt + assoc;
                }
                // debug
                // for (int j=0; j<64; j++) {
                //     if (j == 0) {
                //         std::cout << "Data: (" << std::hex << tempData[j] << " ";
                //     }
                //     else if (j == 63) {
                //         std::cout << std::hex << tempData[j] << ")\n";
                //     }
                //     else {
                //         std::cout << std::hex << tempData[j] << " ";
                //     }
                // }
                // for (int j=0; j<64; j++) {
                //     if (j == 0) {
                //         std::cout << "Datax: (" << std::hex << tempDatax[j];
                //     }
                //     else if (j == 63) {
                //         std::cout << std::hex << tempDatax[j] << ")\n";
                //     }
                //     else {
                //         std::cout << std::hex << tempDatax[j] << " ";
                //     }
                // }
                // Save stats for each cache line
                // beforeCompressedSpace.push_back(bdi(tempData));
                // afterCompressedSpace.push_back(bdi(tempDatax));

                // Print stats (maybe add file output)
                // std::cout << std::dec << "Before: -" << bdi(tempData) << "- vs After: -" << bdi(tempDatax) << "-\n";

                way_count++;
                actual_cache_lines++;
            }
        }
    }
    // Sum up the vectors containing the compression stats for every cache line
    int beforeSum = std::accumulate(beforeCompressedSpace.begin(), beforeCompressedSpace.end(), 0);
    int afterSum = std::accumulate(afterCompressedSpace.begin(), afterCompressedSpace.end(), 0);
    //Computing cache utilization
    cacheUtil = float(actual_cache_lines) / float(cache_line_num);
    // Computing way utilization
    wayUtil = float(actual_way_cnt) / float(max_way_cnt);
    // std::cout << "\n";   // debug
    std::tuple<int, int, double, double> compressionStats = std::make_tuple(beforeSum, afterSum, cacheUtil, wayUtil);

    // debug
    // std::cout << "\n(One Access Cache Total) Before Sum: -" << std::dec << beforeSum << "- vs After Sum: -" << afterSum << "-\n";

    return compressionStats;
}

void Cache::snapshot(int cache_num)
{
    std::cout << "Cache: " << cache_num << "\n\n";
    for (uint i=0; i<sets.size(); i++) {
        int way_count = 0;
        std::cout << "Cache Row #" << std::dec << i << " \n";
        for (auto it = sets[i].begin(); it != sets[i].end(); ++it) {
            std::cout << "Way #" << std::dec << way_count << ", Tag: " << std::hex << it->tag << ", Mapping: " << std::dec << it->mapping << "\nData: (";
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
        std::cout << "Entry #" << std::dec << i << "\n";
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