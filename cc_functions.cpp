#include <fstream>
#include <iostream>
#include <iomanip>
#include <iterator>
#include <sstream>
#include <vector>

using namespace std;

bool local = true;
bool debug = true;
bool files = true;
bool bd_debug = false;
bool bdi_debug = false;

inline bool file_exists (const std::string& name) {
    ifstream f(name.c_str());
    return f.good();
}

string infile_generation(const std::string method, const std::string suite, const std::string benchmark, const std::string size, const int updates, const std::string state)
{

    string file_path;

    if (local) {
        if (state == "before") {
            file_path = "/home/vic/Documents/MultiCacheSim/tests/traces/data/" + state + "_" + std::to_string(updates) + ".out";
        }
        else if (state == "after") {
            file_path = "/home/vic/Documents/MultiCacheSim/tests/traces/data/" + state + "_" + method + "_" + std::to_string(updates) + ".out";
        }
    }
    else {
        if (state == "before") {
            file_path = "/aenao-99/karyofyl/results/mcs/" + suite + "/" + benchmark + "/" + size + "/compressibility/" + state + "_" + std::to_string(updates) + ".out";
        }
        else if (state == "after") {
            file_path = "/aenao-99/karyofyl/results/mcs/" + suite + "/" + benchmark + "/" + size + "/compressibility/" + state + "_" + method + "_" + std::to_string(updates) + ".out";
        }
    }

    return file_path;
}

string outfile_generation(const std::string method, const std::string suite, const std::string benchmark, const std::string size, const int updates, const std::string state, const std::string technique)
{

    string file_path;

    if (local) {
        if (state == "size") {
            file_path = "/home/vic/Documents/MultiCacheSim/tests/traces/data/" + technique + "/" + technique + "_" + state + "_" + method + ".out";
        }
    }
    else {
        if (state == "size") {
            file_path = "/aenao-99/karyofyl/results/mcs/" + suite + "/" + benchmark + "/" + size + "/compressibility/" + technique + "/" + technique + "_" + state + "_" + method + ".out";
        }
    }

    return file_path;
}

int bd(const std::string uncompressed_path, const std::string state)
{

    if (debug && bd_debug) cout << "\nBeggining Base+Delta\n";

    // Input file
    std::ifstream bd_uncompressed_trace(uncompressed_path.c_str());

    std::vector<int> beforeSpaceTaken;
    std::vector<int> afterSpaceTaken;

    // Read entire cache
    std::vector<std::array<int,64>> uncompressedData;

    std::string line;

    if (debug && bd_debug) cout << "Starting to read input file:" << uncompressed_path << "\n";

    // Reading data from input file
    while(getline(bd_uncompressed_trace,line))
    {
        istringstream iss(line);
        std::array<int,64> lineData;
        int value;
        for (int i=0; i<64; i++) {
            iss >> std::hex >> value;
            lineData[i] = value;
        }
        uncompressedData.push_back(lineData);
    }

    if (debug && bd_debug) cout << "Finished reading input file:" << uncompressed_path << "\n";

    // Implementing Base-Delta-Immediate
    // Steps:
        // Splitting the cache lines into 2 or 4 or 8 byte chunks
        // First chunk is the base (B0)
        // Deducting other chunks from base
        // Checking if criteria are met (zero bytes)

    // Iterate over every cache line (saved in uncompressedData)
    for (uint i=0; i<uncompressedData.size(); i++) {

        // Qualifying configuration flags
        bool flag81 = true;
        bool flag82 = true;
        bool flag84 = true;
        bool flag41 = true;
        bool flag42 = true;
        bool flag21 = true;

        // 8-byte values
        int counter_in_8 = 0;
        int counter_out_8 = 0;
        std::array<std::array<int,8>,8> values8;

        // 4-byte values
        int counter_in_4 = 0;
        int counter_out_4 = 0;
        std::array<std::array<int,4>,16> values4;

        // 2-byte values
        int counter_in_2 = 0;
        int counter_out_2 = 0;
        std::array<std::array<int,2>,32> values2;

        if (debug && bd_debug) cout << "\nLine #" << std::dec << i << "\n\n";

        for (uint j=0; j<64; j++) {

            if (debug && bd_debug) cout << "\nElement #" << std::dec << j << "/64\n\n";

            /////////////////
            /* 8-byte base */
            /////////////////

            // Populating values8 arrays

            if (counter_out_8 < 8 && counter_in_8 < 8) {
                values8[counter_out_8][counter_in_8] = uncompressedData[i][j];
                if (debug && bd_debug) cout << "values8 [" << std::dec << counter_out_8 << "][" << std::dec << counter_in_8 << "] = " << std::hex << uncompressedData[i][j] << "\n";
                if (counter_in_8 < 7) {
                    counter_in_8++;
                }
                else {
                    counter_in_8 = 0;
                }
                if (j != 0 && j % 8 == 7) counter_out_8++;
                if (debug && bd_debug) cout << "next counter_out_8 = " << std::dec << counter_out_8 << "\n";;
                if (debug && bd_debug) cout << "next counter_in_8 = " << std::dec << counter_in_8 << "\n";
            }

            /////////////////
            /* 4-byte base */
            /////////////////

            // Populating values4 arrays

            if (counter_out_4 < 16 && counter_in_4 < 4) {
                values4[counter_out_4][counter_in_4] = uncompressedData[i][j];
                if (debug && bd_debug) cout << "values4 [" << std::dec << counter_out_4 << "][" << std::dec << counter_in_4 << "] = " << std::hex << uncompressedData[i][j] << "\n";
                if (counter_in_4 < 3) {
                    counter_in_4++;
                }
                else {
                    counter_in_4 = 0;
                }
                if (j != 0 && j % 4 == 3) counter_out_4++;
                if (debug && bd_debug) cout << "next counter_out_4 = " << std::dec << counter_out_4 << "\n";;
                if (debug && bd_debug) cout << "next counter_in_4 = " << std::dec << counter_in_4 << "\n";
            }

            /////////////////
            /* 2-byte base */
            /////////////////

            // Populating values2 arrays

            if (counter_out_2 < 32 && counter_in_2 < 2) {
                values2[counter_out_2][counter_in_2] = uncompressedData[i][j];
                if (debug && bd_debug) cout << "values2 [" << std::dec << counter_out_2 << "][" << std::dec << counter_in_2 << "] = " << std::hex << uncompressedData[i][j] << "\n";
                if (counter_in_2 < 1) {
                    counter_in_2++;
                }
                else {
                    counter_in_2 = 0;
                }
                if (j != 0 && j % 2 == 1) counter_out_2++;
                if (debug && bd_debug) cout << "next counter_out_2 = " << std::dec << counter_out_2 << "\n";;
                if (debug && bd_debug) cout << "next counter_in_2 = " << std::dec << counter_in_2 << "\n";
            }

        }

        /////////////////
        /* 8-byte base */
        /////////////////

        std::array<std::array<int,8>,7> differences8;

        // Computing the differences (8-byte)
        for (uint i=1; i<8; i++) {
            for (uint j=0; j<8; j++) {
                differences8[i-1][j] = values8[0][j] - values8[i][j];
                if (debug && bd_debug) cout << "differences8" << "[" << std::dec << i-1 << "][" << j << "] = " << "values8[0][" << j << "] - values8[" << i << "][" << j << "] / " \
                    << differences8[i-1][j] << " (" << values8[0][j] << " - " << values8[i][j] << ")\n";
            }
        }

        if (debug && bd_debug) cout << "\n";

        // Checking the differences (8-byte)
        for (uint i=0; i<7; i++) {
            for (uint j=0; j<8; j++) {
                // Delta = 1-byte
                if (j <= 6) {
                    if (differences8[i][j] != 0) flag81 = false;
                }
                // Delta = 2-byte
                if (j <= 5) {
                    if (differences8[i][j] != 0) flag82 = false;
                }
                // Delta = 4-byte
                if (j <= 3) {
                    if (differences8[i][j] != 0) flag84 = false;
                }
            }
        }

        if (debug && bd_debug) cout << "flag81 = " << std::boolalpha << flag81 << "\n";
        if (debug && bd_debug) cout << "flag82 = " << std::boolalpha << flag82 << "\n";
        if (debug && bd_debug) cout << "flag84 = " << std::boolalpha << flag84 << "\n";
        if (debug && bd_debug) cout << "\n";

        /////////////////
        /* 4-byte base */
        /////////////////

        std::array<std::array<int,4>,15> differences4;

        // Computing the differences (4-byte)
        for (uint i=1; i<16; i++) {
            for (uint j=0; j<4; j++) {
                differences4[i-1][j] = values4[0][j] - values4[i][j];
                if (debug && bd_debug) cout << "differences4" << "[" << std::dec << i-1 << "][" << j << "] = " << "values4[0][" << j << "] - values4[" << i << "][" << j << "] / " \
                    << differences4[i-1][j] << " (" << values4[0][j] << " - " << values4[i][j] << ")\n";
            }
        }

        if (debug && bd_debug) cout << "\n";

        // Checking the differences (4-byte)
        for (uint i=0; i<15; i++) {
            for (uint j=0; j<4; j++) {
                // Delta = 1-byte
                if (j <= 2) {
                    if (differences4[i][j] != 0) flag41 = false;
                }
                // Delta = 2-byte
                if (j <= 1) {
                    if (differences4[i][j] != 0) flag42 = false;
                }
            }
        }

        if (debug && bd_debug) cout << "flag41 = " << std::boolalpha << flag41 << "\n";
        if (debug && bd_debug) cout << "flag42 = " << std::boolalpha << flag42 << "\n";
        if (debug && bd_debug) cout << "\n";

        /////////////////
        /* 2-byte base */
        /////////////////

        std::array<std::array<int,2>,31> differences2;

        // Computing the differences (2-byte)
        for (uint i=1; i<32; i++) {
            for (uint j=0; j<2; j++) {
                differences2[i-1][j] = values2[0][j] - values2[i][j];
                if (debug && bd_debug) cout << "differences2" << "[" << std::dec << i-1 << "][" << j << "] = " << "values2[0][" << j << "] - values2[" << i << "][" << j << "] / " \
                    << differences2[i-1][j] << " (" << values2[0][j] << " - " << values2[i][j] << ")\n";
            }
        }

        if (debug && bd_debug) cout << "\n";

        // Checking the differences (2-byte)
        for (uint i=0; i<31; i++) {
            for (uint j=0; j<2; j++) {
                // Delta = 1-byte
                if (j <= 0) {
                    if (differences2[i][j] != 0) flag21 = false;
                }
            }
        }

        if (debug && bd_debug) cout << "flag21 = " << std::boolalpha << flag21 << "\n";
        if (debug && bd_debug) cout << "\n";

        if (debug && bd_debug) cout << "Populating " << state << "SpaceTaken array\n";
        if (state == "before") {
            // Choosing the best combination (before)
            if (flag81) {
                beforeSpaceTaken.push_back(16);
            }
            else if (flag41) {
                beforeSpaceTaken.push_back(20);
            }
            else if (flag82) {
                beforeSpaceTaken.push_back(24);
            }
            else if (flag21) {
                beforeSpaceTaken.push_back(34);
            }
            else if (flag42) {
                beforeSpaceTaken.push_back(36);
            }
            else if (flag84) {
                beforeSpaceTaken.push_back(40);
            }
            else {
                beforeSpaceTaken.push_back(64);
            }
        }
        else if (state == "after") {
            // Choosing the best combination (after)
            if (flag81) {
                afterSpaceTaken.push_back(16);
            }
            else if (flag41) {
                afterSpaceTaken.push_back(20);
            }
            else if (flag82) {
                afterSpaceTaken.push_back(24);
            }
            else if (flag21) {
                afterSpaceTaken.push_back(34);
            }
            else if (flag42) {
                afterSpaceTaken.push_back(36);
            }
            else if (flag84) {
                afterSpaceTaken.push_back(40);
            }
            else {
                afterSpaceTaken.push_back(64);
            }
        }
        if (debug && bd_debug) {
            if (state == "before") {
                cout << "beforeSpaceTaken = " << beforeSpaceTaken[i] << "\n\n";
            }
            else if (state == "after") {
                cout << "afterSpaceTaken = " << afterSpaceTaken[i] << "\n\n";
            }
        }
    }

    // Iterate over spaceTaken to see the size of the compressed cache
    // Save it in a file 

    int uncompressedSpace = 0;
    int compressedSpace = 0;
    if (debug && bd_debug) cout << "Computing the " << state << "SpaceTaken after compression\n";
    if (state == "before") {
        for (uint i=0; i<beforeSpaceTaken.size(); i++) {
            uncompressedSpace = uncompressedSpace + 64;
            compressedSpace = compressedSpace + beforeSpaceTaken[i];
        }
    }
    else if (state == "after") {
        for (uint i=0; i<afterSpaceTaken.size(); i++) {
            uncompressedSpace = uncompressedSpace + 64;
            compressedSpace = compressedSpace + afterSpaceTaken[i];
        }
    }

    if (debug && bd_debug) cout << "Uncompressed vs Compressed: " << uncompressedSpace << " - " << compressedSpace << "\n";
    return compressedSpace;
}

int bdi(const std::string uncompressed_path, const std::string state)
{

    if (debug && bdi_debug) cout << "\nBeggining Base-Delta-Immediate\n";

    // Input file
    std::ifstream bdi_uncompressed_trace(uncompressed_path.c_str());

    std::vector<int> beforeSpaceTaken;
    std::vector<int> afterSpaceTaken;

    // Read entire cache
    std::vector<std::array<int,64>> uncompressedData;

    std::string line;

    if (debug && bdi_debug) cout << "Starting to read input file:" << uncompressed_path << "\n";

    // Reading data from input file
    while(getline(bdi_uncompressed_trace,line))
    {
        istringstream iss(line);
        std::array<int,64> lineData;
        int value;
        for (int i=0; i<64; i++) {
            iss >> std::hex >> value;
            lineData[i] = value;
        }
        uncompressedData.push_back(lineData);
    }

    if (debug && bdi_debug) cout << "Finished reading input file:" << uncompressed_path << "\n";

    // Implementing Base-Delta-Immediate

    // Iterate over every cache line (saved in uncompressedData)
    for (uint i=0; i<uncompressedData.size(); i++) {

        // Qualifying configuration flags (for zero bases)
        // Also keeping track which bases are compressible with zero bases and which are not (true: compressible, false: not compressible)
        bool zero_flag81[8];
        bool zero_flag82[8];
        bool zero_flag84[8];;
        bool zero_flag41[16];
        bool zero_flag42[16];
        bool zero_flag21[32];

        for (uint i=0; i<32; i++) {
            if (i < 8) {
                zero_flag81[i] = true;
                zero_flag82[i] = true;
                zero_flag84[i] = true;
            }
            if (i < 16) {
                zero_flag41[i] = true;
                zero_flag42[i] = true;
            }
            zero_flag21[i] = true;
        }      

        // Qualifying configuration flags
        bool flag81 = true;
        bool flag82 = true;
        bool flag84 = true;
        bool flag41 = true;
        bool flag42 = true;
        bool flag21 = true;

        // Bases
        // Non-zero-parts: used to find the base (if non_zero_parts is larger than 1, it means that the base was already found)
        // (8-byte, delta = 4-byte)
        std::array<int,8> base84;
        int non_zero_parts84 = 0;
        // (8-byte, delta = 2-byte)
        std::array<int,8> base82;
        int non_zero_parts82 = 0;
        // (8-byte, delta = 1-byte)
        std::array<int,8> base81;
        int non_zero_parts81 = 0;
        // (4-byte, delta = 2-byte)
        std::array<int,4> base42;
        int non_zero_parts42 = 0;
        // (4-byte, delta = 1-byte)
        std::array<int,4> base41;
        int non_zero_parts41 = 0;
        // (2-byte, delta = 1-byte)
        std::array<int,2> base21;
        int non_zero_parts21 = 0;

        // 8-byte values
        int counter_in_8 = 0;
        int counter_out_8 = 0;
        std::array<std::array<int,8>,8> values8;

        // 4-byte values
        int counter_in_4 = 0;
        int counter_out_4 = 0;
        std::array<std::array<int,4>,16> values4;

        // 2-byte values
        int counter_in_2 = 0;
        int counter_out_2 = 0;
        std::array<std::array<int,2>,32> values2;

        // Checking if the entire line is zeros
        bool zeroes = true;

        // Checking for a repeated pattern
        bool pattern = true;
        std::array<int,8> pattern_array;

        // if (debug && bdi_debug) cout << "\nLine #" << std::dec << i << "\n\n";

        for (uint j=0; j<64; j++) {

            // if (debug && bdi_debug) cout << "\nElement #" << std::dec << j << "/64\n\n";

            if (uncompressedData[i][j] != 0) zeroes = false;

            /////////////////
            /* 8-byte base */
            /////////////////

            // Populating values8 arrays

            if (counter_out_8 < 8 && counter_in_8 < 8) {
                values8[counter_out_8][counter_in_8] = uncompressedData[i][j];
                // if (debug && bdi_debug) cout << "values8 [" << std::dec << counter_out_8 << "][" << std::dec << counter_in_8 << "] = " << std::hex << uncompressedData[i][j] << "\n";
                if (counter_in_8 < 7) {
                    counter_in_8++;
                }
                else {
                    counter_in_8 = 0;
                }
                if (j != 0 && j % 8 == 7) counter_out_8++;
                // if (debug && bdi_debug) cout << "next counter_out_8 = " << std::dec << counter_out_8 << "\n";;
                // if (debug && bdi_debug) cout << "next counter_in_8 = " << std::dec << counter_in_8 << "\n";
            }

            /////////////////
            /* 4-byte base */
            /////////////////

            // Populating values4 arrays

            if (counter_out_4 < 16 && counter_in_4 < 4) {
                values4[counter_out_4][counter_in_4] = uncompressedData[i][j];
                // if (debug && bdi_debug) cout << "values4 [" << std::dec << counter_out_4 << "][" << std::dec << counter_in_4 << "] = " << std::hex << uncompressedData[i][j] << "\n";
                if (counter_in_4 < 3) {
                    counter_in_4++;
                }
                else {
                    counter_in_4 = 0;
                }
                if (j != 0 && j % 4 == 3) counter_out_4++;
                // if (debug && bdi_debug) cout << "next counter_out_4 = " << std::dec << counter_out_4 << "\n";;
                // if (debug && bdi_debug) cout << "next counter_in_4 = " << std::dec << counter_in_4 << "\n";
            }

            /////////////////
            /* 2-byte base */
            /////////////////

            // Populating values2 arrays

            if (counter_out_2 < 32 && counter_in_2 < 2) {
                values2[counter_out_2][counter_in_2] = uncompressedData[i][j];
                // if (debug && bdi_debug) cout << "values2 [" << std::dec << counter_out_2 << "][" << std::dec << counter_in_2 << "] = " << std::hex << uncompressedData[i][j] << "\n";
                if (counter_in_2 < 1) {
                    counter_in_2++;
                }
                else {
                    counter_in_2 = 0;
                }
                if (j != 0 && j % 2 == 1) counter_out_2++;
                // if (debug && bdi_debug) cout << "next counter_out_2 = " << std::dec << counter_out_2 << "\n";;
                // if (debug && bdi_debug) cout << "next counter_in_2 = " << std::dec << counter_in_2 << "\n";
            }

        }


        for (uint i=0; i<8; i++) {
            for (uint j=0; j<8; j++) {
                if (i == 0) {
                    pattern_array[j] = values8[i][j];
                    // if (debug && bdi_debug) cout << "pattern_array[" << j << "] = values8[" << i << "][" << j << "] = " << std::hex << values8[i][j] << "\n";
                }
                else {
                    // if (debug && bdi_debug) cout << "pattern_array[" << j << "] = values8[" << i << "][" << j << "] = " << std::hex << values8[i][j] << "\n";
                    if (pattern_array[j] != values8[i][j]) pattern = false;
                    // if (debug && bdi_debug) cout << "Pattern check is " << std::boolalpha << pattern << "\n";
                    if (!pattern) break;
                }
            }
            if (!pattern) break;
        }

        // if (debug && bdi_debug) cout << "Final Pattern check is " << std::boolalpha << pattern << "\n\n";

        ///////////////////////////
        /* Zero base (8-byte) */
        ///////////////////////////

        std::array<std::array<int,8>,8> zero_differences8;

        // Computing the differences with the zero base (8-byte)
        for (uint i=0; i<8; i++) {
            for (uint j=0; j<8; j++) {
                zero_differences8[i][j] = values8[i][j] - 0;
                // if (debug && bdi_debug) cout << "zero_differences8" << "[" << std::dec << i << "][" << j << "] = " << "values8[" << i << "][" << j << "] - 0" \
                //     << " / " << zero_differences8[i][j] << " (" << values8[i][j] << " - " << 0 << ")\n";
            }
        }

        // if (debug && bdi_debug) cout << "\n";

        // Checking the differences (8-byte)
        for (uint i=0; i<8; i++) {
            for (uint j=0; j<8; j++) {
                // Delta = 1-byte
                if (j <= 6) {
                    if (zero_differences8[i][j] != 0) {
                        zero_flag81[i] = false;
                    }
                    // if (debug && bdi_debug) cout << "zero_flag81[" << i << "] is " << std::boolalpha << zero_flag81[i] << " at j = " << j << "\n";
                }
                // Delta = 2-byte
                if (j <= 5) {
                    if (zero_differences8[i][j] != 0) {
                        zero_flag82[i] = false;
                    }
                    // if (debug && bdi_debug) cout << "zero_flag82[" << i << "] is " << std::boolalpha << zero_flag82[i] << " at j = " << j << "\n";
                }
                // Delta = 4-byte
                if (j <= 3) {
                    if (zero_differences8[i][j] != 0) {
                        zero_flag84[i] = false;
                    }
                    // if (debug && bdi_debug) cout << "zero_flag84[" << i << "] is " << std::boolalpha << zero_flag84[i] << " at j = " << j << "\n";
                }
            }
            // if (debug && bdi_debug) cout << "\nzero_flag81[" << i << "] = " << std::boolalpha << zero_flag81[i] << "\n";
            // if (debug && bdi_debug) cout << "zero_flag82[" << i << "] = " << std::boolalpha << zero_flag82[i] << "\n";
            // if (debug && bdi_debug) cout << "zero_flag84[" << i << "] = " << std::boolalpha << zero_flag84[i] << "\n\n";
        }

        // Find base for Base+Delta
        // Delta = 4-byte
        for (uint i=0; i<8; i++) {
            if (!zero_flag84[i] && non_zero_parts84 == 0) {
                for (uint j=0; j<8; j++) {
                    base84[j] = values8[i][j];
                }
            }
            if (!zero_flag84[i]) non_zero_parts84++;
        }
        // if (debug && bdi_debug) {
        //     cout << "base84 = ";
        //     for (uint j=0; j<8; j++) {
        //         cout << std::hex << base84[j] << " ";
        //     }
        //     cout << "\n";
        // }
        // if (debug && bdi_debug) cout << "non_zero_parts84 = " << non_zero_parts84 << "\n";

        // Delta = 2-byte
        for (uint i=0; i<8; i++) {
            if (!zero_flag82[i] && non_zero_parts82 == 0) {
                for (uint j=0; j<8; j++) {
                    base82[j] = values8[i][j];
                }
            }
            if (!zero_flag82[i]) non_zero_parts82++;
        }
        // if (debug && bdi_debug) {
        //     cout << "base82 = ";
        //     for (uint j=0; j<8; j++) {
        //         cout << std::hex << base82[j] << " ";
        //     }
        //     cout << "\n";
        // }
        // if (debug && bdi_debug) cout << "non_zero_parts82 = " << non_zero_parts82 << "\n";

        // Delta = 1-byte
        for (uint i=0; i<8; i++) {
            if (!zero_flag81[i] && non_zero_parts81 == 0) {
                for (uint j=0; j<8; j++) {
                    base81[j] = values8[i][j];
                }
            }
            if (!zero_flag81[i]) non_zero_parts81++;
        }
        // if (debug && bdi_debug) {
        //     cout << "base81 = ";
        //     for (uint j=0; j<8; j++) {
        //         cout << std::hex << base81[j] << " ";
        //     }
        //     cout << "\n";
        // }
        // if (debug && bdi_debug) cout << "non_zero_parts81 = " << non_zero_parts81 << "\n";

        ///////////////////////////
        /* Zero base (4-byte) */
        ///////////////////////////

        std::array<std::array<int,4>,16> zero_differences4;

        // Computing the differences with the zero base (4-byte)
        for (uint i=0; i<16; i++) {
            for (uint j=0; j<4; j++) {
                zero_differences4[i][j] = values4[i][j] - 0;
                // if (debug && bdi_debug) cout << "zero_differences4" << "[" << std::dec << i << "][" << j << "] = " << "values4[" << i << "][" << j << "] - 0" \
                //     << " / " << zero_differences4[i][j] << " (" << values4[i][j] << " - " << 0 << ")\n";
            }
        }

        // if (debug && bdi_debug) cout << "\n";

        // Checking the differences (4-byte)
        for (uint i=0; i<16; i++) {
            for (uint j=0; j<4; j++) {
                // Delta = 1-byte
                if (j <= 2) {
                    if (zero_differences4[i][j] != 0) {
                        zero_flag41[i] = false;
                    }
                    // if (debug && bdi_debug) cout << "zero_flag41[" << i << "] is " << std::boolalpha << zero_flag41[i] << " at j = " << j << "\n";
                }
                // Delta = 2-byte
                if (j <= 1) {
                    if (zero_differences4[i][j] != 0) {
                        zero_flag42[i] = false;
                    }
                    // if (debug && bdi_debug) cout << "zero_flag42[" << i << "] is " << std::boolalpha << zero_flag42[i] << " at j = " << j << "\n";
                }
            }
            // if (debug && bdi_debug) cout << "\nzero_flag41[" << i << "] = " << std::boolalpha << zero_flag41[i] << "\n";
            // if (debug && bdi_debug) cout << "zero_flag42[" << i << "] = " << std::boolalpha << zero_flag42[i] << "\n\n";
        }

        // Find base for Base+Delta
        // Delta = 2-byte
        for (uint i=0; i<16; i++) {
            if (!zero_flag42[i] && non_zero_parts42 == 0) {
                for (uint j=0; j<4; j++) {
                    base42[j] = values4[i][j];
                }
            }
            if (!zero_flag42[i]) non_zero_parts42++;
        }
        // if (debug && bdi_debug) {
        //     cout << "base42 = ";
        //     for (uint j=0; j<4; j++) {
        //         cout << std::hex << base42[j] << " ";
        //     }
        //     cout << "\n";
        // }
        // if (debug && bdi_debug) cout << "non_zero_parts42 = " << non_zero_parts42 << "\n";

        // Delta = 1-byte
        for (uint i=0; i<16; i++) {
            if (!zero_flag41[i] && non_zero_parts41 == 0) {
                for (uint j=0; j<4; j++) {
                    base41[j] = values4[i][j];
                }
            }
            if (!zero_flag41[i]) non_zero_parts41++;
        }
        // if (debug && bdi_debug) {
        //     cout << "base41 = ";
        //     for (uint j=0; j<4; j++) {
        //         cout << std::hex << base41[j] << " ";
        //     }
        //     cout << "\n";
        // }
        // if (debug && bdi_debug) cout << "non_zero_parts41 = " << non_zero_parts41 << "\n";

        ///////////////////////////
        /* Zero base (2-byte) */
        ///////////////////////////

        std::array<std::array<int,2>,32> zero_differences2;

        // Computing the differences with the zero base (2-byte)
        for (uint i=0; i<32; i++) {
            for (uint j=0; j<2; j++) {
                zero_differences2[i][j] = values2[i][j] - 0;
                // if (debug && bdi_debug) cout << "zero_differences2" << "[" << std::dec << i << "][" << j << "] = " << "values2[" << i << "][" << j << "] - 0" \
                //     << " / " << zero_differences2[i][j] << " (" << values2[i][j] << " - " << 0 << ")\n";
            }
        }

        // if (debug && bdi_debug) cout << "\n";

        // Checking the differences (8-byte)
        for (uint i=0; i<32; i++) {
            for (uint j=0; j<2; j++) {
                // Delta = 1-byte
                if (j <= 0) {
                    if (zero_differences2[i][j] != 0) {
                        zero_flag21[i] = false;
                    }
                    // if (debug && bdi_debug) cout << "zero_flag21[" << i << "] is " << std::boolalpha << zero_flag21[i] << " at j = " << j << "\n";
                }
            }
            // if (debug && bdi_debug) cout << "\nzero_flag21[" << i << "] = " << std::boolalpha << zero_flag21[i] << "\n\n";
        }

        // Find base for Base+Delta
        // Delta = 1-byte
        for (uint i=0; i<32; i++) {
            if (!zero_flag21[i] && non_zero_parts21 == 0) {
                for (uint j=0; j<2; j++) {
                    base21[j] = values2[i][j];
                }
            }
            if (!zero_flag21[i]) non_zero_parts21++;
        }
        // if (debug && bdi_debug) {
        //     cout << "base21 = ";
        //     for (uint j=0; j<2; j++) {
        //         cout << std::hex << base21[j] << " ";
        //     }
        //     cout << "\n";
        // }
        // if (debug && bdi_debug) cout << "non_zero_parts21 = " << non_zero_parts21 << "\n";

        // if (debug && bdi_debug) cout << "\n";

        /////////////////
        /* 8-byte base */
        /////////////////

        std::vector<std::array<int,8>> differences81;
        std::vector<std::array<int,8>> differences82;
        std::vector<std::array<int,8>> differences84;

        // Base counters are used so that we skip the base when computing the differences 
        // (Don't compute the difference of the base with itself)
        bool base_counter81 = false;
        bool base_counter82 = false;
        bool base_counter84 = false;

        // Temporary array used for feeding the cache part into the differences vectors
        std::array<int,8> temporaryDifferences8;

        // Computing the differences (8-byte / delta = 1-byte)
        for (uint i=0; i<8; i++) {
            if (!zero_flag81[i]) {
                if (!base_counter81) {
                    base_counter81 = true;
                }
                else {
                    for (uint j=0; j<8; j++) {
                        temporaryDifferences8[j] = base81[j] - values8[i][j];
                    }
                    differences81.push_back(temporaryDifferences8);
                }
            }
        }

        // Computing the differences (8-byte / delta = 2-byte)
        for (uint i=0; i<8; i++) {
            if (!zero_flag82[i]) {
                if (!base_counter82) {
                    base_counter82 = true;
                }
                else {
                    for (uint j=0; j<8; j++) {
                        temporaryDifferences8[j] = base82[j] - values8[i][j];
                    }
                    differences82.push_back(temporaryDifferences8);
                }
            }
        }

        // Computing the differences (8-byte / delta = 4-byte)
        for (uint i=0; i<8; i++) {
            if (!zero_flag84[i]) {
                if (!base_counter84) {
                    base_counter84 = true;
                }
                else {
                    for (uint j=0; j<8; j++) {
                        temporaryDifferences8[j] = base84[j] - values8[i][j];
                    }
                    differences84.push_back(temporaryDifferences8);
                }
            }
        }

        // if (debug && bdi_debug) {
        //     for (uint i=0; i<differences84.size(); i++) {
        //         for (uint j=0; j<8; j++) {
        //             cout << "differences84[" << i << "][" << j << "] = " << std::dec << differences84[i][j] << "\n";
        //         }
        //     }
        //     cout << "\n";
        //     for (uint i=0; i<differences82.size(); i++) {
        //         for (uint j=0; j<8; j++) {
        //             cout << "differences82[" << i << "][" << j << "] = " << std::dec << differences82[i][j] << "\n";
        //         }
        //     }
        //     cout << "\n";
        //     for (uint i=0; i<differences81.size(); i++) {
        //         for (uint j=0; j<8; j++) {
        //             cout << "differences81[" << i << "][" << j << "] = " << std::dec << differences81[i][j] << "\n";
        //         }
        //     }
        //     cout << "\n";
        // }

        // Checking the differences only if there are more than one non-zero parts (otherwise it's only the arbitrary value base)
        // Checking the differences (8-byte / delta = 1-byte)
        if (non_zero_parts81 > 1) {
            for (uint i=0; i<differences81.size(); i++) {
                for (uint j=0; j<8; j++) {
                    if (j <= 6) {
                        if (differences81[i][j] != 0) flag81 = false;
                    }
                }
            }
        }

        // Checking the differences (8-byte / delta = 2-byte)
        if (non_zero_parts82 > 1) {
            for (uint i=0; i<differences82.size(); i++) {
                for (uint j=0; j<8; j++) {
                    if (j <= 5) {
                        if (differences82[i][j] != 0) flag82 = false;
                    }
                }
            }
        }

        // Checking the differences (8-byte / delta = 4-byte)
        if (non_zero_parts84 > 1) {
            for (uint i=0; i<differences84.size(); i++) {
                for (uint j=0; j<8; j++) {
                    if (j <= 3) {
                        if (differences84[i][j] != 0) flag84 = false;
                    }
                }
            }
        }

        // if (debug && bdi_debug) cout << "flag81 = " << std::boolalpha << flag81 << "\n";
        // if (debug && bdi_debug) cout << "flag82 = " << std::boolalpha << flag82 << "\n";
        // if (debug && bdi_debug) cout << "flag84 = " << std::boolalpha << flag84 << "\n";
        // if (debug && bdi_debug) cout << "\n";

        /////////////////
        /* 4-byte base */
        /////////////////

        std::vector<std::array<int,4>> differences41;
        std::vector<std::array<int,4>> differences42;

        // Base counters are used so that we skip the base when computing the differences 
        // (Don't compute the difference of the base with itself)
        bool base_counter41 = false;
        bool base_counter42 = false;

        // Temporary array used for feeding the cache part into the differences vectors
        std::array<int,4> temporaryDifferences4;

        // Computing the differences (4-byte / delta = 1-byte)
        for (uint i=0; i<16; i++) {
            if (!zero_flag41[i]) {
                if (!base_counter41) {
                    base_counter41 = true;
                }
                else {
                    for (uint j=0; j<4; j++) {
                        temporaryDifferences4[j] = base41[j] - values4[i][j];
                    }
                    differences41.push_back(temporaryDifferences4);
                }
            }
        }

        // Computing the differences (4-byte / delta = 2-byte)
        for (uint i=0; i<16; i++) {
            if (!zero_flag42[i]) {
                if (!base_counter42) {
                    base_counter42 = true;
                }
                else {
                    for (uint j=0; j<4; j++) {
                        temporaryDifferences4[j] = base42[j] - values4[i][j];
                    }
                    differences42.push_back(temporaryDifferences4);
                }
            }
        }

        // if (debug && bdi_debug) {
        //     for (uint i=0; i<differences42.size(); i++) {
        //         for (uint j=0; j<4; j++) {
        //             cout << "differences42[" << i << "][" << j << "] = " << std::dec << differences42[i][j] << "\n";
        //         }
        //     }
        //     cout << "\n";
        //     for (uint i=0; i<differences41.size(); i++) {
        //         for (uint j=0; j<4; j++) {
        //             cout << "differences41[" << i << "][" << j << "] = " << std::dec << differences41[i][j] << "\n";
        //         }
        //     }
        //     cout << "\n";
        // }

        // Checking the differences only if there are more than one non-zero parts (otherwise it's only the arbitrary value base)
        // Checking the differences (4-byte / delta = 1-byte)
        if (non_zero_parts41 > 1) {
            for (uint i=0; i<differences41.size(); i++) {
                for (uint j=0; j<4; j++) {
                    if (j <= 2) {
                        if (differences41[i][j] != 0) flag41 = false;
                    }
                }
            }
        }

        // Checking the differences (8-byte / delta = 2-byte)
        if (non_zero_parts42 > 1) {
            for (uint i=0; i<differences42.size(); i++) {
                for (uint j=0; j<4; j++) {
                    if (j <= 1) {
                        if (differences42[i][j] != 0) flag42 = false;
                    }
                }
            }
        }

        // if (debug && bdi_debug) cout << "flag41 = " << std::boolalpha << flag41 << "\n";
        // if (debug && bdi_debug) cout << "flag42 = " << std::boolalpha << flag42 << "\n";
        // if (debug && bdi_debug) cout << "\n";

        /////////////////
        /* 2-byte base */
        /////////////////

        std::vector<std::array<int,2>> differences21;

        // Base counters are used so that we skip the base when computing the differences 
        // (Don't compute the difference of the base with itself)
        bool base_counter21 = false;

        // Temporary array used for feeding the cache part into the differences vectors
        std::array<int,2> temporaryDifferences2;

        // Computing the differences (2-byte / delta = 1-byte)
        for (uint i=0; i<32; i++) {
            if (!zero_flag21[i]) {
                if (!base_counter21) {
                    base_counter21 = true;
                }
                else {
                    for (uint j=0; j<2; j++) {
                        temporaryDifferences2[j] = base21[j] - values2[i][j];
                    }
                    differences21.push_back(temporaryDifferences2);
                }
            }
        }

        // if (debug && bdi_debug) {
        //     for (uint i=0; i<differences21.size(); i++) {
        //         for (uint j=0; j<2; j++) {
        //             cout << "differences21[" << i << "][" << j << "] = " << std::dec << differences21[i][j] << "\n";
        //         }
        //     }
        //     cout << "\n";
        // }

        // Checking the differences only if there are more than one non-zero parts (otherwise it's only the arbitrary value base)
        // Checking the differences (2-byte / delta = 1-byte)
        if (non_zero_parts21 > 1) {
            for (uint i=0; i<differences21.size(); i++) {
                for (uint j=0; j<2; j++) {
                    if (j <= 0) {
                        if (differences21[i][j] != 0) flag21 = false;
                    }
                }
            }
        }

        // if (debug && bdi_debug) cout << "flag21 = " << std::boolalpha << flag21 << "\n";
        // if (debug && bdi_debug) cout << "\n";

        // Calculating the space taken by the cache line after BDI has been implemented
        // if (debug && bdi_debug) cout << "Populating " << state << "SpaceTaken array\n";
        if (state == "before") {
            // Choosing the best combination (before)
            if (zeroes) {
                beforeSpaceTaken.push_back(1);
            }
            else if (pattern) {
                beforeSpaceTaken.push_back(8);
            }
            else if (flag81) {
                beforeSpaceTaken.push_back(16);
            }
            else if (flag41) {
                beforeSpaceTaken.push_back(20);
            }
            else if (flag82) {
                beforeSpaceTaken.push_back(24);
            }
            else if (flag21) {
                beforeSpaceTaken.push_back(34);
            }
            else if (flag42) {
                beforeSpaceTaken.push_back(36);
            }
            else if (flag84) {
                beforeSpaceTaken.push_back(40);
            }
            else {
                beforeSpaceTaken.push_back(64);
            }
        }
        else if (state == "after") {
            // Choosing the best combination (after)
            if (zeroes) {
                afterSpaceTaken.push_back(1);
            }
            else if (pattern) {
                afterSpaceTaken.push_back(8);
            }
            else if (flag81) {
                afterSpaceTaken.push_back(16);
            }
            else if (flag41) {
                afterSpaceTaken.push_back(20);
            }
            else if (flag82) {
                afterSpaceTaken.push_back(24);
            }
            else if (flag21) {
                afterSpaceTaken.push_back(34);
            }
            else if (flag42) {
                afterSpaceTaken.push_back(36);
            }
            else if (flag84) {
                afterSpaceTaken.push_back(40);
            }
            else {
                afterSpaceTaken.push_back(64);
            }
        }
        // if (debug && bdi_debug) {
        //     if (state == "before") {
        //         cout << "beforeSpaceTaken = " << beforeSpaceTaken[i] << "\n\n";
        //     }
        //     else if (state == "after") {
        //         cout << "afterSpaceTaken = " << afterSpaceTaken[i] << "\n\n";
        //     }
        // }
    }

    // Iterate over spaceTaken to see the size of the compressed cache
    // Save it in a file 

    int uncompressedSpace = 0;
    int compressedSpace = 0;
    // if (debug && bdi_deb ug) cout << "Computing the " << state << "SpaceTaken after compression\n";
    if (state == "before") {
        for (uint i=0; i<beforeSpaceTaken.size(); i++) {
            uncompressedSpace = uncompressedSpace + 64;
            compressedSpace = compressedSpace + beforeSpaceTaken[i];
        }
    }
    else if (state == "after") {
        for (uint i=0; i<afterSpaceTaken.size(); i++) {
            uncompressedSpace = uncompressedSpace + 64;
            compressedSpace = compressedSpace + afterSpaceTaken[i];
        }
    }

    // if (debug && bdi_debug) cout << "Uncompressed vs Compressed: " << uncompressedSpace << " - " << compressedSpace << "\n";
    return compressedSpace;
}

int main(int argc, char *argv[]){

    string method = "xor";
    string benchmark = "test";
    string suite = "parsec";
    string size = "small";
    string technique = "bd";

    for (int i=0; i<argc; i++) {
        if (std::string(argv[i]) == "-m") {
            method = argv[i+1];
        }
        else if (std::string(argv[i]) == "-b") {
            benchmark = argv[i+1];
        }
        else if (std::string(argv[i]) == "-s") {
            suite = argv[i+1];
        }
        else if (std::string(argv[i]) == "-t") {
            size = argv[i+1];
        }
        else if (std::string(argv[i]) == "-c") {
            technique = argv[i+1];
        }
    }

    string state;
    int updates = 0;

    // Before
    state = "before";
    string before_path = infile_generation(method, suite, benchmark, size, updates, state);

    // After
    state = "after";
    string after_path = infile_generation(method, suite, benchmark, size, updates, state);

    std::vector<int> beforeCompressedSpace;
    std::vector<int> afterCompressedSpace;

    while (file_exists(before_path)){

        if (debug && files) {
            cout << "\nShould be reading: before_" << updates << ".out\n";
            cout << "Reading (before): " << before_path << "\n";
            cout << "Should be reading: after_" << method << "_" << updates << ".out\n";
            cout << "Reading (after): " << after_path << "\n\n";
        }


        // Call compression function with input (uncompressed) + output (compressed) file paths as arguments
        // This needs to be done for both before and after cache states
        if (technique == "bd") {
            state = "before";
            beforeCompressedSpace.push_back(bd(before_path, state));
            state = "after";
            afterCompressedSpace.push_back(bd(after_path, state));
        }
        else if (technique == "bdi") {
            state = "before";
            beforeCompressedSpace.push_back(bdi(before_path, state));
            state = "after";
            afterCompressedSpace.push_back(bdi(after_path, state));
        }

        // New files
        updates++;

        state = "before";
        before_path = infile_generation(method, suite, benchmark, size, updates, state);

        state = "after";
        after_path = infile_generation(method, suite, benchmark, size, updates, state);

    }

    // Compresion technique output (size)
    state = "size";
    string technique_size_path = outfile_generation(method, suite, benchmark, size, updates, state, technique);
    std::ofstream technique_size_trace(technique_size_path.c_str());

    technique_size_trace << "Before After\n";
    for (uint i=0; i<beforeCompressedSpace.size(); i++) {
        technique_size_trace << std::dec << beforeCompressedSpace[i] << " " << std::dec << afterCompressedSpace[i] << "\n";
        if (debug) cout << "\nCompressed Before[" << i << "]: " << std::dec << beforeCompressedSpace[i] << " vs Compressed After[" << i << "]: " << std::dec << afterCompressedSpace[i] << "\n";
    }

    cout << "Finished with no error\n";

}