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

#include "prefetch.h"
#include "system.h"

int AdjPrefetch::prefetchMiss(uint64_t address, unsigned int tid, std::array<int,64> data, std::string precomp_method, std::string precomp_update_method, std::string comp_method, \
    int entries, std::string infinite_freq, std::string hit_update, std::string ignore_i_bytes, int data_type, int bytes_ignored, int sim_threshold, System& sys)
{
   sys.memAccess(address + (1 << sys.setShift), AccessType::Prefetch, data, tid, precomp_method, precomp_update_method, comp_method, entries, infinite_freq, hit_update, ignore_i_bytes, data_type, \
    bytes_ignored, sim_threshold);
   return 1;
}

// Called to check for prefetches in the case of a cache miss.
int SeqPrefetch::prefetchMiss(uint64_t address, unsigned int tid, std::array<int,64> data, std::string precomp_method, std::string precomp_update_method, std::string comp_method, \
    int entries, std::string infinite_freq, std::string hit_update, std::string ignore_i_bytes, int data_type, int bytes_ignored, int sim_threshold, System& sys)
{
   uint64_t set = (address & sys.setMask) >> sys.setShift;
   uint64_t tag = address & sys.tagMask;
   uint64_t lastSet = (lastMiss & sys.setMask) >> sys.setShift;
   uint64_t lastTag = lastMiss & sys.tagMask;
   int prefetched = 0;

   if(tag == lastTag && (lastSet+1) == set) {
      for(uint64_t i=0; i < prefetchNum; i++) {
         prefetched++;
         // Call memAccess to resolve the prefetch. The address is 
         // incremented in the set portion of its bits (least
         // significant bits not in the cache line offset portion)
         sys.memAccess(address + ((1 << sys.setShift) * (i+1)), AccessType::Prefetch, data, tid, precomp_method, precomp_update_method, comp_method, entries, infinite_freq, hit_update, \
            ignore_i_bytes, data_type, bytes_ignored, sim_threshold);
      }
      
      lastPrefetch = address + (1 << sys.setShift);
   }

   lastMiss = address;
   return prefetched;
}

int AdjPrefetch::prefetchHit(uint64_t address, unsigned int tid, std::array<int,64> data, std::string precomp_method, std::string precomp_update_method, std::string comp_method, \
    int entries, std::string infinite_freq, std::string hit_update, std::string ignore_i_bytes, int data_type, int bytes_ignored, int sim_threshold, System& sys)
{
   sys.memAccess(address + (1 << sys.setShift), AccessType::Prefetch, data, tid, precomp_method, precomp_update_method, comp_method, entries, infinite_freq, hit_update, ignore_i_bytes, \
    data_type, bytes_ignored, sim_threshold);
   return 1;
}

// Called to check for prefetches in the case of a cache hit.
int SeqPrefetch::prefetchHit(uint64_t address, unsigned int tid, std::array<int,64> data, std::string precomp_method, std::string precomp_update_method, std::string comp_method, \
    int entries, std::string infinite_freq, std::string hit_update, std::string ignore_i_bytes, int data_type, int bytes_ignored, int sim_threshold, System& sys)
{
   uint64_t set = (address & sys.setShift) >> sys.setShift;
   uint64_t tag = address & sys.tagMask;
   uint64_t lastSet = (lastPrefetch & sys.setMask) 
                                    >> sys.setShift;
   uint64_t lastTag = lastPrefetch & sys.tagMask;

   if(tag == lastTag && lastSet == set) {
      // Call memAccess to resolve the prefetch. The address is 
      // incremented in the set portion of its bits (least
      // significant bits not in the cache line offset portion)
      sys.memAccess(address + ((1 << sys.setShift) * prefetchNum), AccessType::Prefetch, data, tid, precomp_method, precomp_update_method, comp_method, entries, infinite_freq, hit_update, \
        ignore_i_bytes, data_type, bytes_ignored, sim_threshold);
      lastPrefetch = lastPrefetch + (1 << sys.setShift);
   }

   return 1;
}

