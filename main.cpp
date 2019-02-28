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
#include <iosfwd>
#include <fstream>
#include <cassert>
#include <limits>
#include <sstream>
#include <string>
/* Cluster */
#include "/aenao-99/karyofyl/zstr/src/zstr.hpp"
#include "/aenao-99/karyofyl/zstr/src/strict_fstream.hpp"
/* Local */
// #include "/home/vic/zstr/src/zstr.hpp"
// #include "/home/vic/zstr/src/strict_fstream.hpp"

#include "system.h"

using namespace std;

int main(int argc, char* argv[])
{
    std::string method;
    int frequency = 0;
    std::string benchmark = "test";
    int bits_ignored = 0;
    std::string suite = "parsec";
    int entries = 8;
    std::string size = "small";
    unsigned int trace_accesses_start = 0;
    unsigned long long trace_accesses_end = std::numeric_limits<unsigned long long>::max();
    cout << "trace accesses: " << trace_accesses_end << "\n";

    for (int i=0; i<argc; i++) {
        if (std::string(argv[i]) == "-m") {
            method = argv[i+1];
        }
        else if (std::string(argv[i]) == "-f") {
            frequency = atoi(argv[i+1]);
        }
        else if (std::string(argv[i]) == "-b") {
            benchmark = argv[i+1];
        }
        else if (std::string(argv[i]) == "-i") {
            bits_ignored = atoi(argv[i+1]);
        }
        else if (std::string(argv[i]) == "-s") {
            suite = argv[i+1];
        }
        else if (std::string(argv[i]) == "-x") {
            entries = atoi(argv[i+1]);
        }
        else if (std::string(argv[i]) == "-t") {
            size = argv[i+1];
        }
        else if (std::string(argv[i]) == "-l") {
            trace_accesses_start = atoi(argv[i+1]);
            trace_accesses_end = atoi(argv[i+2]);
        }
    }

    if (trace_accesses_end <= trace_accesses_start) {
        cout << "! The end of the simulation window is earlier than the start. Reseted to the end of the trace file.\n";
        trace_accesses_end = std::numeric_limits<unsigned long long>::max();
    }

    cout << "\nInput Stats\n_________________\n\n";
    cout << "Suite: " << suite << "\nBenchmark: " << benchmark << "\nSize: " << size << "\nBits ignored: " << bits_ignored << "\nEntries: " << entries << "\nFrequency: " << frequency\
     << "\n" << "_________________" << "\n\n";

   // tid_map is used to inform the simulator how
   // thread ids map to NUMA/cache domains. Using
   // the tid as an index gives the NUMA domain.
   unsigned int arr_map[] = {0, 1};
   vector<unsigned int> tid_map(arr_map, arr_map + 
         sizeof(arr_map) / sizeof(unsigned int));
   std::unique_ptr<SeqPrefetch> prefetch = std::make_unique<SeqPrefetch>();
   // The constructor parameters are:
   // the tid_map, 
   // the cache line size in bytes,
   // number of cache lines, 
   // the associativity,
   // the prefetcher object,
   // whether to count compulsory misses,
   // whether to do virtual to physical translation,
   // and number of caches/domains
   // WARNING: counting compulsory misses doubles execution time
   SingleCacheSystem sys(64, 131072, 16, std::move(prefetch), false, false);
   /* Quick stats for LLC (assuming 64 byte sized lines)*/
   /* 
   2MB -> 32768 lines
   4MB -> 65536 lines
   8MB -> 131072 lines
   */
   char rw;
   uint64_t address;
   unsigned long long lines = 0;

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
   // cout << "Before infile\n";
   // cout << "/aenao-99/karyofyl/results/pin/pinatrace/" << suite << "/" << benchmark << "/" << size << extra1 << type << extra2 << extra3 << "/trace.out.gz\n";

   /* Cluster */
   zstr::ifstream infile("/aenao-99/karyofyl/results/pin/pinatrace/" + suite + "/" + benchmark + "/" + size + extra1 + type + extra2 + extra3 + "/trace.out.gz");
   /* Local */
   // zstr::ifstream infile("/home/vic/Documents/MultiCacheSim/tests/traces/trace.out.gz");
   // // zstr::ifstream infile("/home/vic/Documents/MultiCacheSim/tests/traces/trace_given_data.out.gz");

   // cout << "After infile\n";
   
   // This code works with the output from the 
   // ManualExamples/pinatrace pin tool
   // infile.open("trace.out", ifstream::in);
   // assert(infile.is_open());

   std::string line;
   int writes = 0, updates = 0;

   while(!infile.eof() && (lines >= trace_accesses_start) && lines <= trace_accesses_end)
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

        if (rw == 'W') {
        if ((writes % frequency) == 0 && writes != 0) {
            // Kmeans
            // cout << "Precompression Table Update #" << updates << "\n\n";
            sys.tableUpdate(updates, benchmark, suite, size, entries, method, bits_ignored);
            sys.modifyData(updates, benchmark, suite, size, entries, method, bits_ignored);
            // sys.snapshot();
            updates++;
        }
        writes++;
        }

        sys.checkSimilarity(lineData,bits_ignored,rw);

        ++lines;
   }

   cout << "Updates: " << updates << "\n_________________\n\n";
   cout << "Traditional Stats\n_________________\n\n";

   cout << "Accesses: " << lines << endl;
   cout << "Hits: " << sys.stats.hits << endl;
   cout << "Misses: " << lines - sys.stats.hits << endl;
   cout << "Local reads: " << sys.stats.local_reads << endl;
   cout << "Local writes: " << sys.stats.local_writes << endl;
   cout << "Remote reads: " << sys.stats.remote_reads << endl;
   cout << "Remote writes: " << sys.stats.remote_writes << endl;
   cout << "Other-cache reads: " << sys.stats.othercache_reads << endl;
   //cout << "Compulsory Misses: " << sys.stats.compulsory << endl;
   
   // sys.printSimilarity(bits_ignored, benchmark);

   // infile.close();

   return 0;
}
