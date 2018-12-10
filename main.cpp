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

#include <iostream>
#include <fstream>
#include <cassert>
#include <sstream>
#include <string>
#include "/aenao-99/karyofyl/zstr/src/zstr.hpp"

#include "system.h"

using namespace std;

int main(int argc, char* argv[])
{
   // tid_map is used to inform the simulator how
   // thread ids map to NUMA/cache domains. Using
   // the tid as an index gives the NUMA domain.
   unsigned int arr_map[] = {0, 1};
   vector<unsigned int> tid_map(arr_map, arr_map + 
         sizeof(arr_map) / sizeof(unsigned int));
   std::unique_ptr<SeqPrefetch> prefetch = std::make_unique<SeqPrefetch>();
   // The constructor parameters are:
   // the tid_map, the cache line size in bytes,
   // number of cache lines, the associativity,
   // the prefetcher object,
   // whether to count compulsory misses,
   // whether to do virtual to physical translation,
   // and number of caches/domains
   // WARNING: counting compulsory misses doubles execution time
   SingleCacheSystem sys(64, 16, 2, std::move(prefetch), false, false);
   char rw;
   uint64_t address;
   unsigned long long lines = 0;
   // zstr::ifstream infile("trace.out.gz");
   std::string type;
   std::string benchmark = argv[2];
   if (benchmark == "blackscholes" || benchmark == "bodytrack" || benchmark == "facesim" || benchmark == "ferret" || benchmark == "fluidanimate" || benchmark == "freqmine" || benchmark == "raytrace" ||
    benchmark == "swaptions" || benchmark == "vips" || benchmark == "x264" || benchmakr == "test") {
     type = "apps";
   }
   else if (benchmark == "canneal" || benchmark == "dedup" || benchmark == "streamcluster") {
     type = "kernels";
   }
   zstr::ifstream infile("/aenao-99/karyofyl/results/pin/pinatrace/parsec/" + benchmark + "/small/pkgs/" + type + "/" + benchmark + "/run/trace.out.gz");
   // This code works with the output from the 
   // ManualExamples/pinatrace pin tool
   // infile.open("trace.out", ifstream::in);
   // assert(infile.is_open());

   std::string line;

   while(!infile.eof())
   // while(std::getline(infile, line))
   {
      infile.ignore(256, ':');
      // Reading access
      infile >> rw;
      assert(rw == 'R' || rw == 'W');
      AccessType accessType;
      if (rw == 'R') {
         accessType = AccessType::Read;
      } else {
         accessType = AccessType::Write;
      }
      // Reading address
      infile >> hex >> address;

      // std::vector<int> lineData(64,0);
      std::array<int,64> lineData = {0};
      int value;
      // Reading 64 values (64 bytes)
      for (int i=0; i<64; i++) {
        infile >> hex >> value;
        lineData[i] = value;
      }

      if(address != 0) {
         // By default the pinatrace tool doesn't record the tid,
         // so we make up a tid to stress the MultiCache functionality
         sys.memAccess(address, accessType, lineData, lines%2);
      }

      // if (rw == 'R') {
      //   int x = atoi(argv[1]);
      //   sys.checkSimilarity(lineData,x);
      // }

      int x = atoi(argv[1]);
      sys.checkSimilarity(lineData,x,rw);

      ++lines;
   }

   std::cout << "Traditional Stats\n_________________\n\n";

   cout << "Accesses: " << lines << endl;
   cout << "Hits: " << sys.stats.hits << endl;
   cout << "Misses: " << lines - sys.stats.hits << endl;
   cout << "Local reads: " << sys.stats.local_reads << endl;
   cout << "Local writes: " << sys.stats.local_writes << endl;
   cout << "Remote reads: " << sys.stats.remote_reads << endl;
   cout << "Remote writes: " << sys.stats.remote_writes << endl;
   cout << "Other-cache reads: " << sys.stats.othercache_reads << endl;
   //cout << "Compulsory Misses: " << sys.stats.compulsory << endl;
   
   sys.printSimilarity();

   // infile.close();

   return 0;
}
