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

// Flags for debug printouts;
bool debug = true;
bool trace_accesses = false;
bool snapshot = true;

/* Cluster */
std::string machine = "cluster";
/* Local */
// std::string machine = "local";

std::string tracefile_generation(const std::string option, const std::string suite, const std::string benchmark, const std::string size, const std::string machine)
{
    std::string type;
    std:string file_path;

    /* Options */
    // Pre" is for simulating the cache - gets traces as inputs
    // "Trace" is for generating smaller traces containing only useful accesses

    if (machine == "cluster") {
        if (suite == "parsec") {

            if (benchmark == "blackscholes" || benchmark == "bodytrack" || benchmark == "facesim" || benchmark == "ferret" || benchmark == "fluidanimate" || benchmark == "freqmine" ||
                benchmark == "raytrace" || benchmark == "swaptions" || benchmark == "vips" || benchmark == "x264" || benchmark == "test") {
                // cout << "Into apps\n";
                type = "apps";
            }
            else if (benchmark == "canneal" || benchmark == "dedup" || benchmark == "streamcluster") {
                // cout << "Into kernels\n";
                type = "kernels";
            }

            if (option == "pre"){
                file_path = "/aenao-99/karyofyl/results/pin/pinatrace/parsec/" + benchmark + "/" + size + "/pkgs/" + type + "/" + benchmark + "/run/trace.out.gz";
            }
            else if (option == "trace") {
                file_path = "/aenao-99/karyofyl/results/pin/pinatrace/parsec/" + benchmark + "/" + size + "/pkgs/" + type + "/" + benchmark + "/run/trace_min.out";
            }

        }
        else if (suite == "perfect") {

            if (benchmark == "2d_convolution" || benchmark == "dwt" || benchmark == "histogram_equalization") {
                type = "pa1";
            }
            else if (benchmark == "interp1" || benchmark == "interp2" || benchmark == "backprojection") {
                type = "sar";
            }
            else if (benchmark == "outer_product" || benchmark == "inner_product" || benchmark == "system_solve") {
                type = "stap";
            }
            else if (benchmark == "app" || benchmark == "debayer" || benchmark == "lucas-kanade" || benchmark == "change-detection") {
                type = "wami";
            }

            if (option == "pre"){
                file_path = "/aenao-99/karyofyl/results/pin/pinatrace/perfect/" + type + "/" + benchmark + "/" + size + "/trace.out.gz";
            }
            else if (option == "trace") {
                file_path = "/aenao-99/karyofyl/results/pin/pinatrace/perfect/" + type + "/" + benchmark + "/" + size + "/trace_min.out";
            }

        }
        else if (suite == "phoenix") {
            if (option == "pre"){
                file_path = "/aenao-99/karyofyl/results/pin/pinatrace/phoenix/" + benchmark + "/" + size + "/" + "/trace.out.gz";
            }
            else if (option == "trace") {
                file_path = "/aenao-99/karyofyl/results/pin/pinatrace/phoenix/" + benchmark + "/" + size + "/" + "/trace_min.out";
            }
        }
        else if (suite == "polybench") {
            if (benchmark == "correlation" || benchmark == "covariance") {
                type = "datamining";
            }
            else if (benchmark == "gemm" || benchmark == "gemver" || benchmark == "gesummv" || benchmark == "symm" || benchmark == "syr2k" || benchmark == "syrk" || benchmark == "trmm") {
                type = "linear-algebra/blas";
            }
            else if (benchmark == "2mm" || benchmark == "3mm" || benchmark == "atax" || benchmark == "bicg" || benchmark == "doitgen" || benchmark == "mvt") {
                type = "linear-algebra/kernels";
            }
            else if (benchmark == "cholesky" || benchmark == "durbin" || benchmark == "gramschmidt" || benchmark == "lu" || benchmark == "ludcmp" || benchmark == "trisolv" ) {
                type = "linear-algebra/solvers";
            }
            else if (benchmark == "deriche" || benchmark == "floyd-warshall" || benchmark == "nussinov") {
                type = "medley";
            }
            else if (benchmark == "adi" || benchmark == "fdtd-2d" || benchmark == "heat-3d" || benchmark == "jacobi-1d" || benchmark == "jacobi-2d" || benchmark == "seidel-2d") {
                type = "stencils";
            }

            if (option == "pre"){
                file_path = "/aenao-99/karyofyl/results/pin/pinatrace/polybench/" + type + "/" + benchmark + "/" + size + "/trace.out.gz";
            }
            else if (option == "trace") {
                file_path = "/aenao-99/karyofyl/results/pin/pinatrace/polybench/" + type + "/" + benchmark + "/" + size + "/trace_min.out";
            }
        }
    }
    else if (machine == "local") {
        if (option == "pre"){
            // file_path = "/home/vic/Documents/MultiCacheSim/tests/traces/trace.out.gz";
            file_path = "/home/vic/Documents/MultiCacheSim/tests/traces/trace_one_line.out.gz";
            // file_path = "/home/vic/Documents/MultiCacheSim/tests/traces/trace_one_line_old.out.gz";
        }
        else if (option == "trace") {
            // file_path = "/home/vic/Documents/MultiCacheSim/tests/traces/trace_min.out";
            file_path = "/home/vic/Documents/MultiCacheSim/tests/traces/trace_one_line_min.out";
            // file_path = "/home/vic/Documents/MultiCacheSim/tests/traces/trace_one_line_old_min.out";
        }
    }

    return file_path;
}

int main(int argc, char* argv[])
{

    std::string method;
    int frequency = 1;
    std::string benchmark = "test";
    int bits_ignored = 0;
    std::string suite = "parsec";
    int entries = 8;
    std::string size = "small";
    std::string hit_update = "n";
    std::string option = "pre";
    unsigned int trace_accesses_start = 0;
    unsigned long long trace_accesses_end = std::numeric_limits<unsigned long long>::max();

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
        else if (std::string(argv[i]) == "-u") {
            hit_update = argv[i+1];
        }
        else if (std::string(argv[i]) == "-o") {
            option = argv[i+1];
        }
        else if (std::string(argv[i]) == "-l") {
            trace_accesses_start = atoi(argv[i+1]);
            trace_accesses_end = atoi(argv[i+2]);
        }
    }

    if (option != "pre" && option != "trace"){
        option = "pre";
    }

    if (hit_update != "n" && hit_update != "y"){
        hit_update = "n";
    }

    if (trace_accesses_end <= trace_accesses_start) {
        cout << "Simulation Window:\n";
        trace_accesses_end = std::numeric_limits<unsigned long long>::max();
    }

    if (method != "xor" && method != "add") {
        method = "xor";
    }
    if (!(entries && !(entries & (entries - 1)))) {
        int old_entries = entries;
        entries--;
        entries |= entries >> 1;
        entries |= entries >> 2;
        entries |= entries >> 4;
        entries |= entries >> 8;
        entries |= entries >> 16;
        entries++;
        cout << "The number of precompression table entries (" << old_entries << ") is not a power of two. It has been increased to " << entries << ".\n";
    }
    

    cout << "\nInput Stats\n_________________\n\n";
    cout << "Precompression method: " << method << "\nSuite: " << suite << "\nBenchmark: " << benchmark << "\nSize: " << size << "\nBits ignored: " << bits_ignored << "\nEntries: " << entries << "\nFrequency: " << frequency \
        << "\nOption: " << option << "\nUpdate on write hit: " << hit_update << "\n" << "___________________________________________________" << "\n\n";


    // tid_map is used to inform the simulator how
    // thread ids map to NUMA/cache domains. Using
    // the tid as an index gives the NUMA domain.
    unsigned int arr_map[] = {0, 1};
    vector<unsigned int> tid_map(arr_map, arr_map + 
         sizeof(arr_map) / sizeof(unsigned int));
    std::unique_ptr<SeqPrefetch> prefetch = std::make_unique<SeqPrefetch>();
    // The constructor parameters are:
    // the tid_map, --- (isn't used in Single Cache System)
    // the cache line size in bytes,
    // number of cache lines, 
    // the associativity,
    // the prefetcher object,
    // whether to count compulsory misses,
    // whether to do virtual to physical translation,
    // and number of caches/domains
    // WARNING: counting compulsory misses doubles execution time
    SingleCacheSystem sys(64, 16384, 4, NULL, false, false);
    // MultiCacheSystem sys(tid_map, 64, 2, 2, std::move(prefetch), false, false, 2);
    /* Quick stats for LLC (assuming 64 byte sized lines)*/
    /* 
    1MB -> 16384 lines
    2MB -> 32768 lines
    4MB -> 65536 lines
    8MB -> 131072 lines
    */
    uint64_t pc;
    char rw;
    uint64_t address;
    unsigned long long lines = 0;
    
    std::string pre_option = "pre";
    std::string trace_input = tracefile_generation(pre_option, suite, benchmark, size, machine);
    zstr::ifstream infile(trace_input.c_str());

    std::string trace_option = "trace";
    std::string trace_min_output = tracefile_generation(trace_option, suite, benchmark, size, machine);
    ofstream trace_outfile(trace_min_output.c_str());

    std::string line;
    int writes = 0;

    // trace_info = (set, way, tag, access type, data)
    std::tuple<uint64_t, uint, uint64_t, std::string, std::array<int,64>> trace_info;

    while(getline(infile,line))
    {
        istringstream iss(line);
        iss.ignore(256, ':');

        iss >> rw;
        assert(rw == 'R' || rw == 'W');
        AccessType accessType;
        if (rw == 'R') {
            accessType = AccessType::Read;
        } else {
            accessType = AccessType::Write;
            writes++;
        }
        // Reading address
        iss >> hex >> address;

        std::array<int,64> lineData = {0};
        int value;
        // Reading 64 values (64 bytes)
        for (int i=0; i<64; i++) {
            iss >> hex >> value;
            lineData[i] = value;
        }

        // Restrictring the part of the trace that is being simulated
        if (lines >= trace_accesses_start && lines <= trace_accesses_end) {
            if(address != 0) {
                // By default the pinatrace tool doesn't record the tid,
                // so we make up a tid to stress the MultiCache functionality
                trace_info = sys.memAccess(address, accessType, lineData, lines%2, method, hit_update);
                // Precompression code
                if (option == "pre") {
                    if ((writes % frequency) == 0 && writes != 0) {
                        sys.precompress(method, entries);
                        if (debug && snapshot) {
                            cout << "\n";
                            sys.snapshot();
                        }
                    }
                }
                if (option == "trace") {
                    std::stringstream output_value;
                    output_value << get<0>(trace_info) << " " << get<1>(trace_info) << " " << get<2>(trace_info) << " " << get<3>(trace_info) << " ";
                    for (uint i=0; i<64; i++) {
                        if (i!=63) {
                            output_value << get<4>(trace_info)[i] << " ";
                        }
                        else {
                            output_value << get<4>(trace_info)[i];
                        }
                    }
                    trace_outfile << output_value.str() << "\n";
                }
            }
        }

        ++lines;
    }

    cout << "Accesses: " << lines << endl;
    cout << "Hits: " << sys.stats.hits << endl;
    cout << "Misses: " << lines - sys.stats.hits << endl;
    cout << "Local reads: " << sys.stats.local_reads << endl;
    cout << "Local writes: " << sys.stats.local_writes << endl;
    cout << "Remote reads: " << sys.stats.remote_reads << endl;
    cout << "Remote writes: " << sys.stats.remote_writes << endl;
    cout << "Other-cache reads: " << sys.stats.othercache_reads << endl;
    //cout << "Compulsory Misses: " << sys.stats.compulsory << endl;

    return 0;
}
