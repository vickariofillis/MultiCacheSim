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

namespace std
{
    template<typename T, size_t N>
    struct hash<array<T, N> >
    {
        typedef array<T, N> argument_type;
        typedef size_t result_type;

        result_type operator()(const argument_type& a) const
        {
            hash<T> hasher;
            result_type h = 0;
            for (result_type i = 0; i < N; ++i)
            {
                h = h * 31 + hasher(a[i]);
            }
            return h;
        }
    };
}

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
   void insertLine(uint64_t set, uint64_t tag, CacheState state, std::array<int,64> data);
   void updateData(uint64_t set, uint64_t tag, std::array<int,64> data);
   std::string outfile_generation(std::string function, const std::string method, const std::string suite, const std::string benchmark, const std::string size, const int bits_ignored,\
     const int updates, const std::string state, const int binary_file);
   std::string infile_generation(std::string function, const std::string method, const std::string suite, const std::string benchmark, const std::string size, const int bits_ignored,\
     const int updates, const std::string state, const int binary_file);
   void snapshot(const std::string cacheState);
   int cacheElements();
   void checkSimilarity(std::array<int,64> lineData, int maskedBits, char rw);
   void printSimilarity(const int updates, const std::string benchmark, const std::string suite, const std::string size, const int entries, const std::string method, const int bits_ignored);
   // Kmeans
   bool tableUpdate(const int updates, const std::string benchmark, const std::string suite, const std::string size, const int entries, const std::string method, const int bits_ignored);
   void modifyData(const int updates, const std::string benchmark, const std::string suite, const std::string size, const int entries, const std::string method, const int bits_ignored);
private:
   std::vector<std::deque<CacheLine>> sets;
   unsigned int maxSetSize;
   std::unordered_map<std::array<int,64>, int> occurence;
};

