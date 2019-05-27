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
#include <limits>
#include <numeric>
#include <typeinfo>
#include <stdio.h>
#include <sys/stat.h>

#include "misc.h"
#include "cache.h"
#include "compression.h"

// Kmeans
/* Cluster */
#include "/aenao-99/karyofyl/dkm-master/include/dkm_parallel.hpp"
/* Local */
// #include "/home/vic/Documents/dkm-master/include/dkm_parallel.hpp"

// Flags for output files generation
bool frequent_entries_flag = true;
// Flag for checking if frequent entries files already existed
bool frequent_file_check = false;

// int precomp_updates = 1; // debug

bool fileExists(const std::string& filename)
{
    struct stat buf;
    if (stat(filename.c_str(), &buf) != -1)
    {
        return true;
    }
    return false;
}

// File generation for printing most frequent entries
std::string frequent_entries_outfile_generation(const std::string machine, const std::string suite, const std::string benchmark, const std::string size, const int entries, \
    const std::string infinite_freq, const int frequency_threshold, const int data_type, const int bytes_ignored)
{
    std::string file_path;

    if (machine == "cluster") {
        if (infinite_freq == "y") {
            if (frequency_threshold == 0) {
                file_path = "/aenao-99/karyofyl/results/mcs/" + suite + "/" + benchmark + "/" + size + "/compressibility/infinite_frequent_entries_reset_" + std::to_string(entries) + ".out";
            }
            else if (frequency_threshold == std::numeric_limits<int>::max()) {
                file_path = "/aenao-99/karyofyl/results/mcs/" + suite + "/" + benchmark + "/" + size + "/compressibility/infinite_frequent_entries_max_" + std::to_string(entries) + ".out";
            }
            else {
                file_path = "/aenao-99/karyofyl/results/mcs/" + suite + "/" + benchmark + "/" + size + "/compressibility/infinite_frequent_entries_" + std::to_string(frequency_threshold) \
                    + "_" + std::to_string(entries) + ".out";
            }
        }
        else if (infinite_freq == "n") {
            if (frequency_threshold == 0) {
                file_path = "/aenao-99/karyofyl/results/mcs/" + suite + "/" + benchmark + "/" + size + "/compressibility/finite_frequent_entries_reset_" + std::to_string(entries) + ".out";
            }
            else if (frequency_threshold == std::numeric_limits<int>::max()) {
                file_path = "/aenao-99/karyofyl/results/mcs/" + suite + "/" + benchmark + "/" + size + "/compressibility/finite_frequent_entries_max_" + std::to_string(entries) + ".out";
            }
            else {
                file_path = "/aenao-99/karyofyl/results/mcs/" + suite + "/" + benchmark + "/" + size + "/compressibility/finite_frequent_entries_" + std::to_string(frequency_threshold) \
                    + "_" + std::to_string(entries) + ".out";
            }
        }
    }
    else if (machine == "local") {
        if (infinite_freq == "y") {
            if (frequency_threshold == 0) {
                file_path = "/home/vic/Documents/MultiCacheSim/tests/traces/" + suite + "/" + benchmark + "/" + "infinite_frequent_entries_reset_" + std::to_string(entries) + ".out";
            }
            else if (frequency_threshold == std::numeric_limits<int>::max()) {
                file_path = "/home/vic/Documents/MultiCacheSim/tests/traces/" + suite + "/" + benchmark + "/" + "infinite_frequent_entries_max_" + std::to_string(entries) + ".out";
            }
            else {
                file_path = "/home/vic/Documents/MultiCacheSim/tests/traces/" + suite + "/" + benchmark + "/" + "infinite_frequent_entries_" + std::to_string(frequency_threshold) \
                    + "_" + std::to_string(entries) + ".out";
            }
        }
        else if (infinite_freq == "n") {
            if (frequency_threshold == 0) {
                file_path = "/home/vic/Documents/MultiCacheSim/tests/traces/" + suite + "/" + benchmark + "/" + "finite_frequent_entries_reset_" + std::to_string(entries) + ".out";
            }
            else if (frequency_threshold == std::numeric_limits<int>::max()) {
                file_path = "/home/vic/Documents/MultiCacheSim/tests/traces/" + suite + "/" + benchmark + "/" + "finite_frequent_entries_max_" + std::to_string(entries) + ".out";
            }
            else {
                file_path = "/home/vic/Documents/MultiCacheSim/tests/traces/" + suite + "/" + benchmark + "/" + "finite_frequent_entries_" + std::to_string(frequency_threshold) \
                    + "_" + std::to_string(entries) + ".out";
            }
        }
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
    std::string infinite_freq, int frequency_threshold, std::string ignore_i_bytes, int data_type, int bytes_ignored, int sim_threshold)
{
    std::string action;

    // Increasing the freq counter in the appropriate frequency table entry
    action = "d";
    // Update frequency table
    updateFrequencyTable(action, set, tag, data, entries, infinite_freq, frequency_threshold, data_type, bytes_ignored);

    if (sets[set].size() == maxSetSize) {
      sets[set].pop_front();
    }

    // Generic cache line insertion
    // sets[set].emplace_back(tag, state, -1, data, data);

    if (precomp_update_method == "kmeans") {
        sets[set].emplace_back(tag, state, -1, data, data);
    }
    else if (precomp_update_method == "frequency") {

        // Increasing the freq counter in the appropriate frequency table entry
        action = "i";
        // Update frequency table
        updateFrequencyTable(action, set, tag, data, entries, infinite_freq, frequency_threshold, data_type, bytes_ignored);


        // Check if a precompression table entry is similar to the inserted data
        int similarEntry = findPrecompressionEntry(data, precomp_update_method, data_type, bytes_ignored, sim_threshold);

        if (similarEntry > -1) {
            //call precompressDatax
            sets[set].emplace_back(tag, state, similarEntry, data, data);
            precompressDatax(precomp_method, set, tag, data_type, bytes_ignored, ignore_i_bytes);
        }
        else {
            sets[set].emplace_back(tag, state, -1, data, data);
        }
    }
}

// Updating cache data in case of a hit
void Cache::updateData(uint64_t set, uint64_t tag, std::array<int,64> data, std::string precomp_method, std::string precomp_update_method, std::string comp_method, int entries, \
    std::string infinite_freq, int frequency_threshold, std::string hit_update, std::string ignore_i_bytes, int data_type, int bytes_ignored, int sim_threshold)
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
                            it->datax[j] = it->data[j] ^ precompressionTable[it->mapping][j];
                            // xorData(set);    //FIXME
                        }
                    }
                }
            }
            else if (precomp_update_method == "frequency") {

                // Increasing the freq counter in the appropriate frequency table entry
                std::string action = "i";

                // Update frequency table
                updateFrequencyTable(action, set, tag, data, entries, infinite_freq, frequency_threshold, data_type, bytes_ignored);

                if (hit_update == "y") {
                    // Update data
                    for (int j=0; j<64; j++) {
                        it->data[j] = data[j];
                    }
                    /* Update datax */

                    // Check if a precompression table entry is similar to the updated data
                    int similarEntry = findPrecompressionEntry(data, precomp_update_method, data_type, bytes_ignored, sim_threshold);

                    if (similarEntry > -1) {
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

void Cache::updateFrequencyTable(std::string action, uint64_t set, uint64_t tag, std::array<int,64> data, int entries, std::string infinite_freq, int frequency_threshold, int data_type, \
    int bytes_ignored)
{
    // Update frequency table

    // Infinite frequency table
    if (infinite_freq == "y") {

        // Increasing the freq counter in the appropriate frequency table entry
        if (action == "i") {
            // Frequency table is empty
            if (infiniteFrequentLines.size() == 0) {
                infiniteFrequentLines.emplace_back(std::make_tuple(data, 1, 0, 0));
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
                    infiniteFrequentLines.emplace_back(std::make_tuple(data, 1, 0, 0));
                }
            }
        }
        // Decreasing the freq counter in the appropriate frequency table entry
        else if (action == "d") {
            // Iterate on cache
            for (auto it = sets[set].begin(); it != sets[set].end(); ++it) {
                if (it->tag == tag) {
                    // Find similarEntry from precompression table
                    int similarEntry = it->mapping;
                    // Get similarEntry's data
                    std::array<int,64> tempSimilarData;
                    for (uint i=0; i<precompressionTable.size(); i++) {
                        for (int j=0; j<64; j++) {
                            tempSimilarData[j] = precompressionTable[i][j];
                        }
                    }
                    // Decrement its counter from frequency table
                    // exists is used for making sure that the precompression table entry is in the frequency table
                    bool exists = false;
                    for (uint i=0; i<infiniteFrequentLines.size(); i++) {
                        // similar: used here for finding the same precompression table entry in the frequency table
                        // it MUST be in the frequency table
                        bool similar = true;
                        for (int j=0; j<64; j++) {
                            if (std::get<0>(infiniteFrequentLines[i])[j] != tempSimilarData[j]) {
                                similar = false;
                            }
                        }
                        if (similar) {
                            std::get<1>(infiniteFrequentLines[i])--;
                            std::cout << "\n!!! freq was decreased from " << std::get<1>(infiniteFrequentLines[i])+1 << " to " << std::get<1>(infiniteFrequentLines[i]) << "\n";
                            exists = true;
                        }
                    }
                    if (!exists) {
                        std::cout << "ERROR: The entry in the precompression table MUST exists in the frequency table.";
                        abort();
                    }
                }
            }
        }

    }
    // FIXME: Change LRU
    else if (infinite_freq == "n") {

        // Flag for ignoring LSBytes when checking for similarity
        int ignore_flag = data_type;

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
        
    }
}

// Updating precompression table
void Cache::updatePrecompressTable(std::string machine, std::string suite, std::string benchmark, std::string size, int entries, std::string precomp_method, std::string precomp_update_method, \
    std::string infinite_freq, int frequency_threshold, std::string ignore_i_bytes, int data_type, int bytes_ignored, int sim_threshold)
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
        precompressionTable.clear();

        // Updating precompressionTable (centroids)
        std::array<int,64> tempCentroid = {0};
        for (const auto& mean : std::get<0>(dkmData)) {
            for (int i=0; i<64; i++) {
                tempCentroid[i] = mean[i];
            }
            precompressionTable.push_back(tempCentroid);
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

    }

    // Precompression table is a frequency table
    else if (precomp_update_method == "frequency") {

        if (infinite_freq == "y") {

            // Sorting frequency table based on freq of entries
            sort(infiniteFrequentLines.begin(),infiniteFrequentLines.end(),[](std::tuple<std::array<int,64>,int,int,int> &a, std::tuple<std::array<int,64>,int,int,int>& b) \
                { return std::get<1>(a) > std::get<1>(b); });

            // debug
            // std::cout << "\nUPDATE = " << precomp_updates << "\n\n";
            // std::cout << "\nBefore\n\nFrequency Table with size of: " << std::dec << infiniteFrequentLines.size() << "\n";
            // for (uint i=0; i<entries; i++) {
            //     std::cout << "Entry = " << i+1 << "\n";
            //     std::cout << "Data: (";
            //     for (int j=0; j<64; j++) {
            //         if (j != 63) {
            //             std::cout << std::hex << std::get<0>(infiniteFrequentLines[i])[j] << " ";
            //         }
            //         else {
            //             std::cout << std::hex << std::get<0>(infiniteFrequentLines[i])[j] << ")";
            //         }
            //     }
            //     std::cout << " / Freq = " << std::dec << std::get<1>(infiniteFrequentLines[i]) << " / Freq_comp = " << std::get<2>(infiniteFrequentLines[i]) << " / Same_counter = " << \
            //         std::get<3>(infiniteFrequentLines[i]);
            //     std::cout << "\n";
            // }
            // std::cout << "\n";
            // debug

            // Filter through the most frequent cache lines (infinite) table
            // for (uint i=0; i<infiniteFrequentLines.size(); i++) {
            for (uint i=infiniteFrequentLines.size(); i-- > 0;) {

                // Freq has remained the same
                if (std::get<1>(infiniteFrequentLines[i]) == std::get<2>(infiniteFrequentLines[i])) {

                    // if (i<32) std::cout << "Entry #" << i+1 << ": freq = " << std::get<1>(infiniteFrequentLines[i]) << " EQUAL freq_comp = " << std::get<2>(infiniteFrequentLines[i]) << "\n";   // debug
                    // Counter for keeping track of # of updates that freq is stagnant (same_counter) is increased
                    std::get<3>(infiniteFrequentLines[i])++;
                    // if (i<32) std:: cout << "same_counter = " << std::get<3>(infiniteFrequentLines[i]) << "\n";   // debug

                    // Delete entries that their stagnant period has exceeded the allowed one
                    if (std::get<3>(infiniteFrequentLines[i]) > frequency_threshold) {
                        //Checking size of frequency table before removing entries
                        if (infiniteFrequentLines.size() > entries) {
                            // if (i<32) std::cout << "Deleting entry #" << i+1 << "\n"; // debug
                            // if (i<32) {
                            //     std::cout << "with Data: (";
                            //     for (int j=0; j<64; j++) {
                            //         if (j != 63) {
                            //             std::cout << std::hex << std::get<0>(infiniteFrequentLines[i])[j] << " ";
                            //         }
                            //         else {
                            //             std::cout << std::hex << std::get<0>(infiniteFrequentLines[i])[j] << ")";
                            //         }
                            //     }
                            // } // debug
                            // Deleting entry
                            infiniteFrequentLines.erase(infiniteFrequentLines.begin() + i);
                        }
                    }
                }
                // Freq has changed
                else {
                    // if (i<32) std::cout << "Entry #" << i+1 << ": freq = " << std::get<1>(infiniteFrequentLines[i]) << " VS freq_comp (old) = " << std::get<2>(infiniteFrequentLines[i]);   // debug
                    // freq_comp = freq
                    std::get<2>(infiniteFrequentLines[i]) = std::get<1>(infiniteFrequentLines[i]);
                    // if (i<32) std::cout << " / freq_comp (new) = " << std::get<2>(infiniteFrequentLines[i]) << "\n";   // debug
                    // same_counter reset to 0
                    std::get<3>(infiniteFrequentLines[i]) = 0;
                }
            }

            // debug
            // std::cout << "\nAfter\n\nFrequency Table with size of: " << std::dec << infiniteFrequentLines.size() << "\n";
            // for (uint i=0; i<entries; i++) {
            //     std::cout << "Entry = " << i+1 << "\n";
            //     std::cout << "Data: (";
            //     for (int j=0; j<64; j++) {
            //         if (j != 63) {
            //             std::cout << std::hex << std::get<0>(infiniteFrequentLines[i])[j] << " ";
            //         }
            //         else {
            //             std::cout << std::hex << std::get<0>(infiniteFrequentLines[i])[j] << ")";
            //         }
            //     }
            //     std::cout << " / Freq = " << std::dec << std::get<1>(infiniteFrequentLines[i]) << " / Freq_comp = " << std::get<2>(infiniteFrequentLines[i]) << " / Same_counter = " << \
            //         std::get<3>(infiniteFrequentLines[i]);
            //     std::cout << "\n";
            // }
            // std::cout << "\n";
            // std::cout << "_________________________________________________________________________________\n";
            // debug

            // precomp_updates++; // debug

            // Find the n (= entries) most frequent cache lines
            // Sort frequency table
            // sort(infiniteFrequentLines.begin(),infiniteFrequentLines.end(),[](std::tuple<std::array<int,64>,int,int,int> &a, std::tuple<std::array<int,64>,int,int,int>& b) \
            //     { return std::get<1>(a) > std::get<1>(b); });   // FIXME

            // Delete centroids vector contents before updating it
            precompressionTable.clear();

            // end_point is used to make sure that the Precompression table is not filled with frequent lines that do not exist
            int end_point = entries;

            if (infiniteFrequentLines.size() < entries) {
                end_point = infiniteFrequentLines.size();
            }

            // Fill Precompression table with the most frequent cache lines
            for (int i=0; i<end_point; i++) {
                std::array<int,64> tempCentroid;
                for (int j=0; j<64; j++) {
                    tempCentroid[j] = std::get<0>(infiniteFrequentLines[i])[j];
                }
                precompressionTable.push_back(tempCentroid);
            }

            // Printing most frequent entries
            if (frequent_entries_flag) {
                // Generating filepath for printing most frequent entries
                std::string frequent_entries_outfile = frequent_entries_outfile_generation(machine, suite, benchmark, size, entries, infinite_freq, frequency_threshold, data_type, bytes_ignored);
                if (!frequent_file_check) {
                    bool file_exists = fileExists(frequent_entries_outfile);
                    if (file_exists) std::remove(frequent_entries_outfile.c_str());
                    frequent_file_check = true;
                }
                std::ofstream frequent_entries_stream(frequent_entries_outfile.c_str(), std::ofstream::out | std::ofstream::app);

                for (int i=0; i<end_point; i++) {
                    frequent_entries_stream << std::dec << i << " ";
                    for (int j=0; j<64; j++) {
                        if (j != 63) {
                            frequent_entries_stream << std::dec << std::get<0>(infiniteFrequentLines[i])[j] << " ";
                        }
                        else {
                            frequent_entries_stream << std::dec << std::get<0>(infiniteFrequentLines[i])[j] << " " << std::dec << std::get<1>(infiniteFrequentLines[i]) << "\n";
                        }
                    }
                }
            }

        }
        else if (infinite_freq == "n") {
            // Frequency table doubles as the precompression table

            // Delete centroids vector contents before updating it
            precompressionTable.clear();

            // end_point is used to make sure that the Precompression table is not filled with frequent lines that do not exist
            int end_point = entries;

            if (frequentLines.size() < entries) {
                end_point = frequentLines.size();
            }

            // Fill Precompression table with the most frequent cache lines
            for (int i=0; i<end_point; i++) {
                std::array<int,64> tempCentroid;
                for (int j=0; j<64; j++) {
                    tempCentroid[j] = frequentLines[i][j];
                }
                precompressionTable.push_back(tempCentroid);
            }

            // Printing most frequent entries
            if (frequent_entries_flag) {
                // Generating filepath for printing most frequent entries
                std::string frequent_entries_outfile = frequent_entries_outfile_generation(machine, suite, benchmark, size, entries, infinite_freq, frequency_threshold, data_type, bytes_ignored);
                if (!frequent_file_check) {
                    bool file_exists = fileExists(frequent_entries_outfile);
                    if (file_exists) std::remove(frequent_entries_outfile.c_str());
                    frequent_file_check = true;
                }
                std::ofstream frequent_entries_stream(frequent_entries_outfile.c_str(), std::ofstream::out | std::ofstream::app);

                for (int i=0; i<end_point; i++) {
                    frequent_entries_stream << std::dec << i << " ";
                    for (int j=0; j<64; j++) {
                        if (j != 63) {
                            frequent_entries_stream << std::dec << frequentLines[i][j] << " ";
                        }
                        else {
                            frequent_entries_stream << std::dec << frequentLines[i][j] << "\n";
                        }
                    }
                }
            }

        }

        // Updating mappings
        for (uint i=0; i<sets.size(); i++) {
            for (auto it = sets[i].begin(); it != sets[i].end(); ++it) {
                std::array<int,64> tempData;
                for (int j=0; j<64; j++) {
                    tempData[j] = it->data[j];
                }
                it->mapping = findPrecompressionEntry(tempData, precomp_update_method, data_type, bytes_ignored, sim_threshold);
            }
        }
    }

    // Compute precompressed data (datax) for entire cache
    precompressCache(precomp_method, data_type, bytes_ignored, ignore_i_bytes);

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
                        it->datax[j] = it->data[j] ^ precompressionTable[it->mapping][j];
                        // xorData(set);    //FIXME
                    }
                }
                ignore_flag--;
                if (ignore_flag == 0) {
                    ignore_flag = data_type;
                }
                else if (ignore_i_bytes == "n") {
                    if (precomp_method == "xor") {
                        it->datax[j] = it->data[j] ^ precompressionTable[it->mapping][j];
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

    // Update datax on the entire cache
    for (uint i=0; i<sets.size(); i++) {
        for (auto it = sets[i].begin(); it != sets[i].end(); ++it) {
            for (int j=0; j<64; j++) {
                if (ignore_i_bytes == "y" && (ignore_flag > bytes_ignored)) {
                    // FIXME: Add other precompression methods
                    if (precomp_method == "xor") {
                        if (it->mapping > -1) it->datax[j] = it->data[j] ^ precompressionTable[it->mapping][j];
                        // std::cout << "it->datax[" << std::dec << j << "] (" << std::hex << it->datax[j] << ") = it->data[" << std::dec << j << "] (" << std::hex << it->data[j] << ") ^ precompressionTable[" \
                        << std::dec << it->mapping << "][" << j << "] (" << std::hex << precompressionTable[it->mapping][j] << ")\n";   // debug
                        if ((it->datax[j]) > 255 || (it->datax[j]) < -255) {
                            // std::cout << "it->datax[" << std::dec << j << "] (" << std::hex << it->datax[j] << ") = it->data[" << std::dec << j << "] (" << std::hex << it->data[j] << ") ^ precompressionTable[" \
                            //     << std::dec << it->mapping << "][" << j << "] (" << std::hex << precompressionTable[it->mapping][j] << ")\n";   // debug
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
                        if (it->mapping > -1) it->datax[j] = it->data[j] ^ precompressionTable[it->mapping][j];
                        // std::cout << "it->datax[" << std::dec << j << "] (" << std::hex << it->datax[j] << ") = it->data[" << std::dec << j << "] (" << std::hex << it->data[j] << ") ^ precompressionTable[" \
                        << std::dec << it->mapping << "][" << j << "] (" << std::hex << precompressionTable[it->mapping][j] << ")\n";   // debug
                        if ((it->datax[j]) > 255 || (it->datax[j]) < -255) {
                            // std::cout << "it->datax[" << std::dec << j << "] (" << std::hex << it->datax[j] << ") = it->data[" << std::dec << j << "] (" << std::hex << it->data[j] << ") ^ precompressionTable[" \
                            //     << std::dec << it->mapping << "][" << j << "] (" << std::hex << precompressionTable[it->mapping][j] << ")\n";   // debug
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
//         it->datax[j] = it->data[j] ^ precompressionTable[it->mapping][j];
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
    int sum_weight[precompressionTable.size()];

    for (uint i=0; i<precompressionTable.size(); i++) {
        sum_weight[i] = 0;
        weight = data_type;
        for (int j=0; j<64; j++) {
            if (precompressionTable[i][j] == data[j]) {
                if (weight > bytes_ignored) {
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

    if (precompressionTable.size() < 1) {
        similarEntry = -1;
    }
    else {

        similarEntry = 0;
        int max = sum_weight[0];

        for (uint i=1; i<precompressionTable.size(); i++) {
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


    if (comp_method == "bdi") {
        // Iterate over cache
        for (uint i=0; i<sets.size(); i++) {
            int way_count = 0;
            for (auto it = sets[i].begin(); it != sets[i].end(); ++it) {
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
    std::tuple<int, int, double, double> compressionStats = std::make_tuple(beforeSum, afterSum, cacheUtil, wayUtil);

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
    for (uint i=0; i<precompressionTable.size(); i++) {
        std::cout << "Entry #" << std::dec << i << "\n";
        std::cout << "(";
        for (int j=0; j<64; j++) {
            if (j != 63) {
                std::cout << std::hex << precompressionTable[i][j] << " ";
            }
            else {
                std::cout << std::hex << precompressionTable[i][j] << ")";
            }
        }
        std::cout << "\n";
    }

    std::cout << "________________________________________________________________________________________________\n\n";

}