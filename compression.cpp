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

#include <fstream>
#include <iostream>
#include <iomanip>
#include <iterator>
#include <sstream>
#include <vector>

#include "compression.h"

bool compression_debug = false;
bool bdi_debug = false;

int bdi(const std::array<int,64> cacheLine) {

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

    // if (compression_debug && bdi_debug) std::cout << "\nLine #" << std::dec << i << "\n\n";

    for (uint j=0; j<64; j++) {

        // if (compression_debug && bdi_debug) std::cout << "\nElement #" << std::dec << j << "/64\n\n";

        if (cacheLine[j] != 0) zeroes = false;

        /////////////////
        /* 8-byte base */
        /////////////////

        // Populating values8 arrays

        if (counter_out_8 < 8 && counter_in_8 < 8) {
            values8[counter_out_8][counter_in_8] = cacheLine[j];
            // if (compression_debug && bdi_debug) std::cout << "values8 [" << std::dec << counter_out_8 << "][" << std::dec << counter_in_8 << "] = " << std::hex << cacheLine[j] << "\n";
            if (counter_in_8 < 7) {
                counter_in_8++;
            }
            else {
                counter_in_8 = 0;
            }
            if (j != 0 && j % 8 == 7) counter_out_8++;
            // if (compression_debug && bdi_debug) std::cout << "next counter_out_8 = " << std::dec << counter_out_8 << "\n";;
            // if (compression_debug && bdi_debug) std::cout << "next counter_in_8 = " << std::dec << counter_in_8 << "\n";
        }

        /////////////////
        /* 4-byte base */
        /////////////////

        // Populating values4 arrays

        if (counter_out_4 < 16 && counter_in_4 < 4) {
            values4[counter_out_4][counter_in_4] = cacheLine[j];
            // if (compression_debug && bdi_debug) std::cout << "values4 [" << std::dec << counter_out_4 << "][" << std::dec << counter_in_4 << "] = " << std::hex << cacheLine[j] << "\n";
            if (counter_in_4 < 3) {
                counter_in_4++;
            }
            else {
                counter_in_4 = 0;
            }
            if (j != 0 && j % 4 == 3) counter_out_4++;
            // if (compression_debug && bdi_debug) std::cout << "next counter_out_4 = " << std::dec << counter_out_4 << "\n";;
            // if (compression_debug && bdi_debug) std::cout << "next counter_in_4 = " << std::dec << counter_in_4 << "\n";
        }

        /////////////////
        /* 2-byte base */
        /////////////////

        // Populating values2 arrays

        if (counter_out_2 < 32 && counter_in_2 < 2) {
            values2[counter_out_2][counter_in_2] = cacheLine[j];
            // if (compression_debug && bdi_debug) std::cout << "values2 [" << std::dec << counter_out_2 << "][" << std::dec << counter_in_2 << "] = " << std::hex << cacheLine[j] << "\n";
            if (counter_in_2 < 1) {
                counter_in_2++;
            }
            else {
                counter_in_2 = 0;
            }
            if (j != 0 && j % 2 == 1) counter_out_2++;
            // if (compression_debug && bdi_debug) std::cout << "next counter_out_2 = " << std::dec << counter_out_2 << "\n";;
            // if (compression_debug && bdi_debug) std::cout << "next counter_in_2 = " << std::dec << counter_in_2 << "\n";
        }

    }


    for (uint i=0; i<8; i++) {
        for (uint j=0; j<8; j++) {
            if (i == 0) {
                pattern_array[j] = values8[i][j];
                // if (compression_debug && bdi_debug) std::cout << "pattern_array[" << j << "] = values8[" << i << "][" << j << "] = " << std::hex << values8[i][j] << "\n";
            }
            else {
                // if (compression_debug && bdi_debug) std::cout << "pattern_array[" << j << "] = values8[" << i << "][" << j << "] = " << std::hex << values8[i][j] << "\n";
                if (pattern_array[j] != values8[i][j]) pattern = false;
                // if (compression_debug && bdi_debug) std::cout << "Pattern check is " << std::boolalpha << pattern << "\n";
                if (!pattern) break;
            }
        }
        if (!pattern) break;
    }

    // if (compression_debug && bdi_debug) std::cout << "Final Pattern check is " << std::boolalpha << pattern << "\n\n";

    ///////////////////////////
    /* Zero base (8-byte) */
    ///////////////////////////

    std::array<std::array<int,8>,8> zero_differences8;

    // Computing the differences with the zero base (8-byte)
    for (uint i=0; i<8; i++) {
        for (uint j=0; j<8; j++) {
            zero_differences8[i][j] = values8[i][j] - 0;
            // if (compression_debug && bdi_debug) std::cout << "zero_differences8" << "[" << std::dec << i << "][" << j << "] = " << "values8[" << i << "][" << j << "] - 0" \
            //     << " / " << zero_differences8[i][j] << " (" << values8[i][j] << " - " << 0 << ")\n";
        }
    }

    // if (compression_debug && bdi_debug) std::cout << "\n";

    // Checking the differences (8-byte)
    for (uint i=0; i<8; i++) {
        for (uint j=0; j<8; j++) {
            // Delta = 1-byte
            if (j <= 6) {
                if (zero_differences8[i][j] != 0) {
                    zero_flag81[i] = false;
                }
                // if (compression_debug && bdi_debug) std::cout << "zero_flag81[" << i << "] is " << std::boolalpha << zero_flag81[i] << " at j = " << j << "\n";
            }
            // Delta = 2-byte
            if (j <= 5) {
                if (zero_differences8[i][j] != 0) {
                    zero_flag82[i] = false;
                }
                // if (compression_debug && bdi_debug) std::cout << "zero_flag82[" << i << "] is " << std::boolalpha << zero_flag82[i] << " at j = " << j << "\n";
            }
            // Delta = 4-byte
            if (j <= 3) {
                if (zero_differences8[i][j] != 0) {
                    zero_flag84[i] = false;
                }
                // if (compression_debug && bdi_debug) std::cout << "zero_flag84[" << i << "] is " << std::boolalpha << zero_flag84[i] << " at j = " << j << "\n";
            }
        }
        // if (compression_debug && bdi_debug) std::cout << "\nzero_flag81[" << i << "] = " << std::boolalpha << zero_flag81[i] << "\n";
        // if (compression_debug && bdi_debug) std::cout << "zero_flag82[" << i << "] = " << std::boolalpha << zero_flag82[i] << "\n";
        // if (compression_debug && bdi_debug) std::cout << "zero_flag84[" << i << "] = " << std::boolalpha << zero_flag84[i] << "\n\n";
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
    // if (compression_debug && bdi_debug) {
    //     std::cout << "base84 = ";
    //     for (uint j=0; j<8; j++) {
    //         std::cout << std::hex << base84[j] << " ";
    //     }
    //     std::cout << "\n";
    // }
    // if (compression_debug && bdi_debug) std::cout << "non_zero_parts84 = " << non_zero_parts84 << "\n";

    // Delta = 2-byte
    for (uint i=0; i<8; i++) {
        if (!zero_flag82[i] && non_zero_parts82 == 0) {
            for (uint j=0; j<8; j++) {
                base82[j] = values8[i][j];
            }
        }
        if (!zero_flag82[i]) non_zero_parts82++;
    }
    // if (compression_debug && bdi_debug) {
    //     std::cout << "base82 = ";
    //     for (uint j=0; j<8; j++) {
    //         std::cout << std::hex << base82[j] << " ";
    //     }
    //     std::cout << "\n";
    // }
    // if (compression_debug && bdi_debug) std::cout << "non_zero_parts82 = " << non_zero_parts82 << "\n";

    // Delta = 1-byte
    for (uint i=0; i<8; i++) {
        if (!zero_flag81[i] && non_zero_parts81 == 0) {
            for (uint j=0; j<8; j++) {
                base81[j] = values8[i][j];
            }
        }
        if (!zero_flag81[i]) non_zero_parts81++;
    }
    // if (compression_debug && bdi_debug) {
    //     std::cout << "base81 = ";
    //     for (uint j=0; j<8; j++) {
    //         std::cout << std::hex << base81[j] << " ";
    //     }
    //     std::cout << "\n";
    // }
    // if (compression_debug && bdi_debug) std::cout << "non_zero_parts81 = " << non_zero_parts81 << "\n";

    ///////////////////////////
    /* Zero base (4-byte) */
    ///////////////////////////

    std::array<std::array<int,4>,16> zero_differences4;

    // Computing the differences with the zero base (4-byte)
    for (uint i=0; i<16; i++) {
        for (uint j=0; j<4; j++) {
            zero_differences4[i][j] = values4[i][j] - 0;
            // if (compression_debug && bdi_debug) std::cout << "zero_differences4" << "[" << std::dec << i << "][" << j << "] = " << "values4[" << i << "][" << j << "] - 0" \
            //     << " / " << zero_differences4[i][j] << " (" << values4[i][j] << " - " << 0 << ")\n";
        }
    }

    // if (compression_debug && bdi_debug) std::cout << "\n";

    // Checking the differences (4-byte)
    for (uint i=0; i<16; i++) {
        for (uint j=0; j<4; j++) {
            // Delta = 1-byte
            if (j <= 2) {
                if (zero_differences4[i][j] != 0) {
                    zero_flag41[i] = false;
                }
                // if (compression_debug && bdi_debug) std::cout << "zero_flag41[" << i << "] is " << std::boolalpha << zero_flag41[i] << " at j = " << j << "\n";
            }
            // Delta = 2-byte
            if (j <= 1) {
                if (zero_differences4[i][j] != 0) {
                    zero_flag42[i] = false;
                }
                // if (compression_debug && bdi_debug) std::cout << "zero_flag42[" << i << "] is " << std::boolalpha << zero_flag42[i] << " at j = " << j << "\n";
            }
        }
        // if (compression_debug && bdi_debug) std::cout << "\nzero_flag41[" << i << "] = " << std::boolalpha << zero_flag41[i] << "\n";
        // if (compression_debug && bdi_debug) std::cout << "zero_flag42[" << i << "] = " << std::boolalpha << zero_flag42[i] << "\n\n";
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
    // if (compression_debug && bdi_debug) {
    //     std::cout << "base42 = ";
    //     for (uint j=0; j<4; j++) {
    //         std::cout << std::hex << base42[j] << " ";
    //     }
    //     std::cout << "\n";
    // }
    // if (compression_debug && bdi_debug) std::cout << "non_zero_parts42 = " << non_zero_parts42 << "\n";

    // Delta = 1-byte
    for (uint i=0; i<16; i++) {
        if (!zero_flag41[i] && non_zero_parts41 == 0) {
            for (uint j=0; j<4; j++) {
                base41[j] = values4[i][j];
            }
        }
        if (!zero_flag41[i]) non_zero_parts41++;
    }
    // if (compression_debug && bdi_debug) {
    //     std::cout << "base41 = ";
    //     for (uint j=0; j<4; j++) {
    //         std::cout << std::hex << base41[j] << " ";
    //     }
    //     std::cout << "\n";
    // }
    // if (compression_debug && bdi_debug) std::cout << "non_zero_parts41 = " << non_zero_parts41 << "\n";

    ///////////////////////////
    /* Zero base (2-byte) */
    ///////////////////////////

    std::array<std::array<int,2>,32> zero_differences2;

    // Computing the differences with the zero base (2-byte)
    for (uint i=0; i<32; i++) {
        for (uint j=0; j<2; j++) {
            zero_differences2[i][j] = values2[i][j] - 0;
            // if (compression_debug && bdi_debug) std::cout << "zero_differences2" << "[" << std::dec << i << "][" << j << "] = " << "values2[" << i << "][" << j << "] - 0" \
            //     << " / " << zero_differences2[i][j] << " (" << values2[i][j] << " - " << 0 << ")\n";
        }
    }

    // if (compression_debug && bdi_debug) std::cout << "\n";

    // Checking the differences (8-byte)
    for (uint i=0; i<32; i++) {
        for (uint j=0; j<2; j++) {
            // Delta = 1-byte
            if (j <= 0) {
                if (zero_differences2[i][j] != 0) {
                    zero_flag21[i] = false;
                }
                // if (compression_debug && bdi_debug) std::cout << "zero_flag21[" << i << "] is " << std::boolalpha << zero_flag21[i] << " at j = " << j << "\n";
            }
        }
        // if (compression_debug && bdi_debug) std::cout << "\nzero_flag21[" << i << "] = " << std::boolalpha << zero_flag21[i] << "\n\n";
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
    // if (compression_debug && bdi_debug) {
    //     std::cout << "base21 = ";
    //     for (uint j=0; j<2; j++) {
    //         std::cout << std::hex << base21[j] << " ";
    //     }
    //     std::cout << "\n";
    // }
    // if (compression_debug && bdi_debug) std::cout << "non_zero_parts21 = " << non_zero_parts21 << "\n";

    // if (compression_debug && bdi_debug) std::cout << "\n";

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

    // if (compression_debug && bdi_debug) {
    //     for (uint i=0; i<differences84.size(); i++) {
    //         for (uint j=0; j<8; j++) {
    //             std::cout << "differences84[" << i << "][" << j << "] = " << std::dec << differences84[i][j] << "\n";
    //         }
    //     }
    //     std::cout << "\n";
    //     for (uint i=0; i<differences82.size(); i++) {
    //         for (uint j=0; j<8; j++) {
    //             std::cout << "differences82[" << i << "][" << j << "] = " << std::dec << differences82[i][j] << "\n";
    //         }
    //     }
    //     std::cout << "\n";
    //     for (uint i=0; i<differences81.size(); i++) {
    //         for (uint j=0; j<8; j++) {
    //             std::cout << "differences81[" << i << "][" << j << "] = " << std::dec << differences81[i][j] << "\n";
    //         }
    //     }
    //     std::cout << "\n";
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

    // if (compression_debug && bdi_debug) std::cout << "flag81 = " << std::boolalpha << flag81 << "\n";
    // if (compression_debug && bdi_debug) std::cout << "flag82 = " << std::boolalpha << flag82 << "\n";
    // if (compression_debug && bdi_debug) std::cout << "flag84 = " << std::boolalpha << flag84 << "\n";
    // if (compression_debug && bdi_debug) std::cout << "\n";

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

    // if (compression_debug && bdi_debug) {
    //     for (uint i=0; i<differences42.size(); i++) {
    //         for (uint j=0; j<4; j++) {
    //             std::cout << "differences42[" << i << "][" << j << "] = " << std::dec << differences42[i][j] << "\n";
    //         }
    //     }
    //     std::cout << "\n";
    //     for (uint i=0; i<differences41.size(); i++) {
    //         for (uint j=0; j<4; j++) {
    //             std::cout << "differences41[" << i << "][" << j << "] = " << std::dec << differences41[i][j] << "\n";
    //         }
    //     }
    //     std::cout << "\n";
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

    // if (compression_debug && bdi_debug) std::cout << "flag41 = " << std::boolalpha << flag41 << "\n";
    // if (compression_debug && bdi_debug) std::cout << "flag42 = " << std::boolalpha << flag42 << "\n";
    // if (compression_debug && bdi_debug) std::cout << "\n";

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

    // if (compression_debug && bdi_debug) {
    //     for (uint i=0; i<differences21.size(); i++) {
    //         for (uint j=0; j<2; j++) {
    //             std::cout << "differences21[" << i << "][" << j << "] = " << std::dec << differences21[i][j] << "\n";
    //         }
    //     }
    //     std::cout << "\n";
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

    // if (compression_debug && bdi_debug) std::cout << "flag21 = " << std::boolalpha << flag21 << "\n";
    // if (compression_debug && bdi_debug) std::cout << "\n";


    // Calculating the space taken by the cache line after BDI has been implemented
    int spaceTaken;

    // if (compression_debug && bdi_debug) std::cout << "Calculating space taken\n";
    // Choosing the best combination
    if (zeroes) {
        spaceTaken = 1;
    }
    else if (pattern) {
        spaceTaken = 8;
    }
    else if (flag81) {
        spaceTaken = 16;
    }
    else if (flag41) {
        spaceTaken = 20;
    }
    else if (flag82) {
        spaceTaken = 24;
    }
    else if (flag21) {
        spaceTaken = 34;
    }
    else if (flag42) {
        spaceTaken = 36;
    }
    else if (flag84) {
        spaceTaken = 40;
    }
    else {
        spaceTaken = 64;
    }

    // if (compression_debug && bdi_debug) std::cout << "Space taken = " << spaceTaken << "\n";
    return spaceTaken;

}