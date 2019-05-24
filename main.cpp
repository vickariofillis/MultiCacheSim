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
bool snapshot = false;

// Flags for output files generation
bool compression_stats_flag = false;

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
            // file_path = "/home/vic/Documents/MultiCacheSim/tests/traces/trace_one_line.out.gz";
            // file_path = "/home/vic/Documents/MultiCacheSim/tests/traces/trace_one_line_old.out.gz";
            file_path = "/home/vic/Documents/MultiCacheSim/tests/traces/" + suite + "/" + benchmark + "/trace.out.gz";
        }
        else if (option == "trace") {
            // file_path = "/home/vic/Documents/MultiCacheSim/tests/traces/trace_min.out";
            // file_path = "/home/vic/Documents/MultiCacheSim/tests/traces/trace_one_line_min.out";
            // file_path = "/home/vic/Documents/MultiCacheSim/tests/traces/trace_one_line_old_min.out";
        }
    }

    return file_path;
}

// File generation for printing compression stats
std::string outfile_generation(const std::string machine, const std::string precomp_method, const std::string precomp_update_method, const std::string comp_method, const int entries, \
    const std::string suite, const std::string benchmark, const std::string size, const std::string hit_update, const std::string ignore_i_bytes, const int data_type, const int bytes_ignored)
{
    std::string file_path;

    if (machine == "cluster") {
        file_path = "/aenao-99/karyofyl/results/mcs/" + suite + "/" + benchmark + "/" + size + "/compressibility/" + precomp_update_method + "_" + comp_method + "_" + \
            std::to_string(entries) + "_" + hit_update + "_" + ignore_i_bytes + "_" + std::to_string(data_type) + "_" + std::to_string(bytes_ignored) + ".out";
    }
    else if (machine == "local") {
        file_path = "/home/vic/Documents/MultiCacheSim/tests/traces/" + suite + "/" + benchmark + "/" + size + "_" + precomp_update_method + "_" + comp_method + "_" + \
            std::to_string(entries) + "_" + hit_update + "_" + ignore_i_bytes + "_" + std::to_string(data_type) + "_" + std::to_string(bytes_ignored) + ".out";
    }
    

    return file_path;
}

void help(){
    // Printing help
    std::cout << "Options\n\n";
    std::cout << "-s:   Benchmark suite selection\n";
    std::cout << "-b:   Benchmark selection\n";
    std::cout << "-t:   Benchmark size\n";
    std::cout << "-o:   Simulator functionality    (pre or trace)\n";
    std::cout << "-m:   Precompression method    (i.e., xor)\n";
    std::cout << "-um:  Precompression table update method    (i.e., kmeans, frequency)\n";
    std::cout << "-cm:  Compression method    (i.e., bdi)\n";
    std::cout << "-f:   Frequency of updating precompression table\n";
    std::cout << "-x:   Entries of precompression table\n";
    std::cout << "-if:  Infinite frequency table    (y or n)\n";
    std::cout << "-ft:  Frequent entry stall threshold\n";
    std::cout << "-u:   Precompress data on a write hit    (y or n)\n";
    std::cout << "-c:   Compute or not parts of line that are ignored during finding similarity    (y or n)\n";
    std::cout << "-d:   Data type size    (i.e., 32 bytes or 16 bytes etc.)\n";
    std::cout << "-i:   LSBytes to ignore when computing similarity    (smaller than data type size)\n";
    std::cout << "-th:  Similarity threshold    (minimum)\n";
    std::cout << "-l:   Start and end of instrumentation (access numbers)\n";
    exit(0);
}

int main(int argc, char* argv[])
{
    std::string suite = "parsec";
    std::string benchmark = "test";
    std::string size = "small";

    std::string option = "pre";
    std::string precomp_method;
    std::string precomp_update_method;
    std::string comp_method;
    
    int frequency = 1;
    int entries = 8;

    std::string infinite_freq = "n";
    int frequency_threshold = std::numeric_limits<int>::max();

    std::string hit_update = "n";
    std::string ignore_i_bytes;
    
    int data_type = 32;
    int bytes_ignored = 0;
    
    int sim_threshold = 0;
    
    unsigned int trace_accesses_start = 0;
    unsigned long long trace_accesses_end = std::numeric_limits<unsigned long long>::max();

    for (int i=0; i<argc; i++) {
        // Help function
        if (std::string(argv[i]) == "-h") {
            help();
        }
        // Precompress data method (i.e., xor)
        if (std::string(argv[i]) == "-m") {
            precomp_method = argv[i+1];
        }
        // Precompression table update method (i.e., frequency of cache block data)
        else if (std::string(argv[i]) == "-um") {
            precomp_update_method = argv[i+1];
        }
        // Compression method (i.e., bdi)
        else if (std::string(argv[i]) == "-cm") {
            comp_method = argv[i+1];
        }
        // Compute precompressed version of data (i.e., datax) for ignored bytes
        else if (std::string(argv[i]) == "-c") {
            ignore_i_bytes = argv[i+1];
        }
        // Frequency of updating the precompression table
        else if (std::string(argv[i]) == "-f") {
            frequency = atoi(argv[i+1]);
        }
        // Entries of precompression table
        else if (std::string(argv[i]) == "-x") {
            entries = atoi(argv[i+1]);
        }
        // Suite the benchmark belongs to
        else if (std::string(argv[i]) == "-s") {
            suite = argv[i+1];
        }
        // Benchmark to be executed
        else if (std::string(argv[i]) == "-b") {
            benchmark = argv[i+1];
        }
        // Size of benchmark to be executed
        else if (std::string(argv[i]) == "-t") {
            size = argv[i+1];
        }
        // Size of data types (size of chunks to divide the cache line into)
        else if (std::string(argv[i]) == "-d") {
            data_type = atoi(argv[i+1]);
        }
        // Bytes to be ignored at the end of every data block
        else if (std::string(argv[i]) == "-i") {
            bytes_ignored = atoi(argv[i+1]);
        }
        // Whether to have an infinite frequency table
        else if (std::string(argv[i]) == "-if") {
            infinite_freq = argv[i+1];
        }
        // # of precompression table updates after which we remove an entry that has remained at a certain frequency
        else if (std::string(argv[i]) == "-ft") {
            frequency_threshold = atoi(argv[i+1]);
        }
        // Whether to update the precompressed data on a cache write hit
        else if (std::string(argv[i]) == "-u") {
            hit_update = argv[i+1];
        }
        // Similarity threshold (must be at least this to assign a cache line to a table entry)
        else if (std::string(argv[i]) == "-th") {
            sim_threshold = atoi(argv[i+1]);
        }
        // Generate minimized traces or actual precompression
        else if (std::string(argv[i]) == "-o") {
            option = argv[i+1];
        }
        // Trace window for benchmark execution
        else if (std::string(argv[i]) == "-l") {
            trace_accesses_start = atoi(argv[i+1]);
            trace_accesses_end = atoi(argv[i+2]);
        }
    }

    // Input checks
    if (option != "pre" && option != "trace"){
        option = "pre";
    }

    if (hit_update != "n" && hit_update != "y"){
        hit_update = "n";
    }

    if (ignore_i_bytes != "n" && ignore_i_bytes != "y"){
        ignore_i_bytes = "n";
    }

    if (trace_accesses_end <= trace_accesses_start) {
        cout << "Simulation Window:\n";
        trace_accesses_end = std::numeric_limits<unsigned long long>::max();
    }

    if (precomp_method != "xor" && precomp_method != "add") {
        precomp_method = "xor";
    }

    if (precomp_update_method != "kmeans" && precomp_update_method != "frequency") {
        precomp_update_method = "frequency";
    }

    if (comp_method != "bdi") {
        comp_method = "bdi";
    }

    if (infinite_freq != "n" && infinite_freq != "y"){
        ignore_i_bytes = "n";
    }

    if (64 % data_type != 0) {
        std::cout << "Data type needs to be a divisor of 64 (cache line size in bytes).\n";
        data_type = 32;
        std::cout << "Reverted data_type to 32 bytes.\n";
    }

    if (bytes_ignored > data_type) {
        std::cout << "Least significant bytes that are ignored cannot be more than the data type size.\n";
        exit(0);
    }
    else if (bytes_ignored > (data_type / 2)) {
        std::cout << "It is recommended that the ignored bytes are at most half of the data type size.\n";
    }

    int max_threshold = (((data_type * (data_type+1)) / 2) * (64 / data_type));

    if (sim_threshold > max_threshold) {
        std::cout << "Similarity threshold is greater than the maximum allowed (" << max_threshold << ") with the chosen data type (" << data_type << " bytes).\n";
        exit(0);
    }

    if (frequency_threshold < 0) {
        std::cout << "Frequency threshold has been set to maximum (no reset).\n";
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
        std::cout << "The number of precompression table entries (" << old_entries << ") is not a power of two. It has been increased to " << entries << ".\n";
    }

    if (precomp_update_method == "frequency" && hit_update == "n") {
        cout << "!!! Precompression table filled with most frequent cache lines (frequency) is highly recommended to be used with updates on datax (precompressed data) in the case of a cache write hit.\n";
    }
    
    // Printing configuration variables
    cout << "\nInput Stats\n___________________________________________________\n\n";
    std::cout << "Suite: " << suite << "\nBenchmark: " << benchmark << "\nSize: " << size << "\nSimulator functionality: " << option << "\nPrecompression method: " << precomp_method << \
        "\nPrecompression table update method: " << precomp_update_method << "\nCompression method: " << comp_method << "\nFrequency: " << frequency << "\nEntries: " << entries << \
        "\nPrecompress on write hit: " << hit_update << "\nInfinite Frequency table: " << infinite_freq << "\nCompute ignored bytes: " << ignore_i_bytes << "\nData type size: " << data_type << \
        "\nBytes ignored: " << bytes_ignored << "\nSimilarity threshold: " << sim_threshold << \
        "\n" << "___________________________________________________" << "\n\n";

    // Number of cache lines (instantiated here for ease-of-use for cache utilization)
    /* Quick stats for LLC (assuming 64 byte sized lines)*/
    /* 
    1MB -> 16384 lines
    2MB -> 32768 lines
    4MB -> 65536 lines
    8MB -> 131072 lines
    */
    int cache_line_num = 16384;
    // Associativity
    int assoc = 4;

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
    SingleCacheSystem sys(64, cache_line_num, assoc, NULL, false, false);
    // MultiCacheSystem sys(tid_map, 64, 2, 2, std::move(prefetch), false, false, 2);
    
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

    // Compression stats of cache(s) (computed after every cache access)
    std::vector<std::tuple<int, int, double, double>> compressionStats;
    // Compression stats of cache(s) (saving stats for the entire trace)
    std::vector<std::vector<std::tuple<int, int, double, double>>> fullCompressionStats;

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
                trace_info = sys.memAccess(address, accessType, lineData, lines%2, precomp_method, precomp_update_method, comp_method, entries, infinite_freq, frequency_threshold, \
                    hit_update, ignore_i_bytes, data_type, bytes_ignored, sim_threshold);
                // Precompression code
                if (option == "pre") {
                    // Call compression calculation algorithm
                    compressionStats = sys.compressStats(cache_line_num, assoc, comp_method);
                    fullCompressionStats.push_back(compressionStats);

                    // debug
                    // Print out cache contents and check compression stats
                    // std::cout << "________________________________________________________________________________________________\n";
                    // std::cout << "----------------------------- NEW ACCESS -----------------------------\n";
                    // std::cout << "\nAccess #" << std::dec << lines << " vs Write #" << writes << "\n";
                    // sys.snapshot();
                    // std::cout << "\n\nCompression Stats\n";
                    // for (uint i=0; i<compressionStats.size(); i++) {
                    //     std::cout << std::dec << "Before: " << get<0>(compressionStats[i]) << " After: " << get<1>(compressionStats[i]) << " Cache Utilization: " << get<2>(compressionStats[i]) \
                    //         << " Way Utilization: " << get<3>(compressionStats[i]) << "\n";
                    // }
                    // std::cout << "________________________________________________________________________________________________\n\n";
                    // debug

                    if ((writes % frequency) == 0 && writes != 0) {
                        sys.precompress(machine, suite, benchmark, size, entries, precomp_method, precomp_update_method, infinite_freq, frequency_threshold, ignore_i_bytes, data_type, \
                            bytes_ignored, sim_threshold);
                        if (debug && snapshot) {
                            cout << "\n";
                            sys.snapshot();
                        }
                    }
                }
                if (option == "trace") {
                    std::stringstream output_value;
                    if (get<3>(trace_info) != "I") {
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
        }

        ++lines;
    }

    // Print compression stats
    long beforeSum = 0;
    long afterSum = 0;
    // Print cache utilization stats
    long statsNum = 0;
    double utilSum = 0.0;
    // Print way utilization stats
    double waySum = 0.0;

    /* Compression Stats */
    /*
    The format of the compression stats is: Before compressed size, After Compressed size, Cache Utilization, Way Utilization
    Every line in the file represents the stats after every access to the cache
    */
    if (compression_stats_flag) {
        // Generating filepath for printing compression stats
        std::string compression_outfile = outfile_generation(machine, precomp_method, precomp_update_method, comp_method, entries, suite, benchmark, size, hit_update, ignore_i_bytes, \
            data_type, bytes_ignored);
        std::ofstream compression_stream(compression_outfile.c_str());

        // Print compression stats
        for (uint i=0; i<fullCompressionStats.size(); i++) {
            for (uint j=0; j<fullCompressionStats[i].size(); j++) {
                if (fullCompressionStats[i].size() == 1) {
                    compression_stream << get<0>(fullCompressionStats[i][j]) << " " << get<1>(fullCompressionStats[i][j]) << " " << get<2>(fullCompressionStats[i][j]) \
                        << " " << get<3>(fullCompressionStats[i][j]);
                }
                else if (fullCompressionStats[i].size() > 1) {
                    compression_stream << get<0>(fullCompressionStats[i][j]) << " " << get<1>(fullCompressionStats[i][j]) << " " << get<2>(fullCompressionStats[i][j]) \
                        << " " << get<3>(fullCompressionStats[i][j]) << " ";
                }
                beforeSum = beforeSum + get<0>(fullCompressionStats[i][j]);
                afterSum = afterSum + get<1>(fullCompressionStats[i][j]);
                statsNum++;
                utilSum = utilSum + get<2>(fullCompressionStats[i][j]);
                waySum = waySum + get<3>(fullCompressionStats[i][j]);

            }
            compression_stream << "\n";
        }
    }
    else  {
        for (uint i=0; i<fullCompressionStats.size(); i++) {
            for (uint j=0; j<fullCompressionStats[i].size(); j++) {
                beforeSum = beforeSum + get<0>(fullCompressionStats[i][j]);
                afterSum = afterSum + get<1>(fullCompressionStats[i][j]);
                statsNum++;
                utilSum = utilSum + get<2>(fullCompressionStats[i][j]);
                waySum = waySum + get<3>(fullCompressionStats[i][j]);
            }
        }
    }


    std::cout << "Before Compressed Byte Size: " << std::dec << beforeSum << "\n";
    std::cout << "After Compressed Byte Size: " << std::dec << afterSum << "\n";
    std::cout << "Before / After Ratio: " << std::dec << (float(afterSum) / float(beforeSum)) << "\n";
    std::cout << "Average Cache Utilization: " << std::dec << (utilSum / statsNum) << "\n";
    std::cout << "Average Way Utilization: " << std::dec << (waySum / statsNum) << "\n\n";

    std::cout << "___________________________________________________\n";

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
