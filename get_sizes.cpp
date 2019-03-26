#include <fstream>
#include <iostream>
#include <iomanip>
#include <iterator>

using namespace std;

inline bool file_exists (const std::string& name) {
    ifstream f(name.c_str());
    return f.good();
}

std::ifstream::pos_type filesize(const char* filename)
{
    std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);
    return in.tellg(); 
}

int main(){

    string method = "xor";
    string state;
    int updates = 0;

    // Before
    state = "before";
    string before_path;
    before_path = "/home/vic/Documents/MultiCacheSim/tests/traces/data/" + state + "_" + std::to_string(updates) + "_binary.out.gz";

    // After
    state = "after";
    string after_path;
    after_path = "/home/vic/Documents/MultiCacheSim/tests/traces/data/" + state + "_" + method + "_" + std::to_string(updates) + "_binary.out.gz";

    // Results
    string results_path = "/home/vic/Documents/MultiCacheSim/tests/traces/data/size_results.out";
    std::ofstream results_trace(results_path.c_str());

    results_trace << "Before After\n";

    while (file_exists(before_path)){

        cout << "Should be reading: before_" << updates << "_binary.out.gz\n";
        cout << "Reading (before): " << before_path << "\n";
        cout << "Should be reading: after_" << updates << "_binary.out.gz\n";
        cout << "Reading (after): " << after_path << "\n";

        results_trace << filesize(before_path.c_str()) << " " << filesize(after_path.c_str()) << "\n";

        // New files
        updates++;
        cout << "Updates = " << updates << "\n";

        // Before
        state = "before";
        before_path = "/home/vic/Documents/MultiCacheSim/tests/traces/data/" + state + "_" + std::to_string(updates) + "_binary.out.gz";

        cout << "New before_path: : " << before_path << "\n";

        // After
        state = "after";
        after_path = "/home/vic/Documents/MultiCacheSim/tests/traces/data/" + state + "_" + method + "_" + std::to_string(updates) + "_binary.out.gz";

        cout << "New after path " << after_path << "\n\n";
    }

    results_trace.close();

}