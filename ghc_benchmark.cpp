#include "include/lockfree.hpp"
#include "include/synclist.hpp"
#include "include/finelock.hpp"
#include "include/utils.h"
#include <cstdio>
#include <cstdlib>
#include <random>
#include <omp.h>
#include <assert.h>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>

using std::vector;
using std::string;
using std::to_string;
using std::cout;

static int _argc;
static const char **_argv;

bool VERBOSE = false;

// get_option functions copy-pasted from wireroute.cpp given in 15-418 asst3 
const char *get_option_string(const char *option_name, const char *default_value) {
    for (int i = _argc - 2; i >= 0; i -= 2)
        if (strcmp(_argv[i], option_name) == 0)
            return _argv[i + 1];
    return default_value;
}

int get_option_int(const char *option_name, int default_value) {
    for (int i = _argc - 2; i >= 0; i -= 2)
        if (strcmp(_argv[i], option_name) == 0)
            return atoi(_argv[i + 1]);
    return default_value;
}

float get_option_float(const char *option_name, float default_value) {
    for (int i = _argc - 2; i >= 0; i -= 2)
        if (strcmp(_argv[i], option_name) == 0)
            return (float)atof(_argv[i + 1]);
    return default_value;
}

/* benchmark_from_inputs takes a given set of keys/operations and input options,
 * and calculates the performance using each implementation
**/
string benchmark_from_inputs(vector<int> &keys, vector<Oper> &ops,
                  double skip_prob, int max_height, int num_trials,
                  int num_threads, int array_length, double update_prob,
                  double removal_prob, std::string dist_info) {
    using namespace std::chrono;
    typedef std::chrono::high_resolution_clock Clock;
    typedef std::chrono::duration<double> dsec;

    int max_deletions = std::count(ops.begin(), ops.end(), 2);

    bool no_sync = false;

    // perform test
    double sync_time = 0;
    double lock_free_time = 0;
    double fine_lock_time = 0;
    if (VERBOSE) cout << "running sync list...";
    if(!no_sync) {
        SyncList<int> *sl = new SyncList<int>(max_height, skip_prob);
        perform_test(sl, keys, ops, array_length, num_threads);
        delete sl;
        for(int i = 0; i < num_trials; i++) {
            sl = new SyncList<int>(max_height, skip_prob);
            auto compute_start = Clock::now();
            perform_test(sl, keys, ops, array_length, num_threads);
            sync_time += duration_cast<dsec>(Clock::now() - compute_start).count();
            delete sl;
        }
        sync_time /= num_trials;
    }
    if (VERBOSE) cout << "done\n Running FineLockList...";
    FineLockList<int> *fl = new FineLockList<int>(max_height, skip_prob, max_deletions);
    perform_test(fl, keys, ops, array_length, num_threads);
    delete fl;
    for(int i = 0; i < num_trials; i++) {
        fl = new FineLockList<int>(max_height, skip_prob, max_deletions);
        auto compute_start = Clock::now();
        perform_test(fl, keys, ops, array_length, num_threads);
        fine_lock_time += duration_cast<dsec>(Clock::now() - compute_start).count();
        delete fl;
    }
    fine_lock_time /= num_trials;
    if (VERBOSE) cout << "done\n Running LockFreeList...";
    LockFreeList<int> *lf = new LockFreeList<int>(max_height, skip_prob, max_deletions);
    perform_test(lf, keys, ops, array_length, num_threads);
    delete lf;
    for(int i = 0; i < num_trials; i++) {
        lf = new LockFreeList<int>(max_height, skip_prob, max_deletions);
        auto compute_start = Clock::now();
        perform_test(lf, keys, ops, array_length, num_threads);
        lock_free_time += duration_cast<dsec>(Clock::now() - compute_start).count();
        delete lf;
    }
    lock_free_time /= num_trials;
    if (VERBOSE) cout << "done\n";
    // put results in csv format
    std::string s = dist_info + "," + 
                    to_string(sync_time) + "," + 
                    to_string(fine_lock_time) + "," + 
                    to_string(lock_free_time) + "," + 
                    to_string(num_threads) + "," +
                    to_string(update_prob) + "," +
                    to_string(removal_prob) + "," +
                    to_string(array_length) + "\n";
    
    return s;
}

int main(int argc, const char *argv[]) {
    using std::ofstream;

    // Get command line arguments
    _argc = argc - 1;
    _argv = argv + 1;
    // skip list probability
    double skip_prob = get_option_float("-p", 0.5f); // probability of increasing a level
    int max_height = get_option_int("-h", 20); // maximum height of skip list

    int num_trials = get_option_int("-r", 5);
    int million = 1000000;
    int ten_million = 10000000;
    int ten_k = 10000;
    int array_length = get_option_int("-a", million);

    double update_prob = get_option_float("-i", 0.1f);
    double removal_prob = get_option_float("-d", 0.1f);
    int variance = get_option_float("-v", ten_k);
    string existing_csv(get_option_string("-f",""));
    string output_fn = "benchmark.csv";
    vector<int> thread_opts = {1,2,4,8};
    cout << std::setprecision(4) << std::fixed;
    bool logging = (bool)get_option_int("--logging", 1);
    VERBOSE = (bool)get_option_int("--verbose", 0);
    if (VERBOSE) {
        cout << "running with verbose\n";
        cout << "update_prob=" << update_prob << ", removal_prob=" << removal_prob;
        cout << ", variance=" << variance << ", array_length=";
        string len_as_string = to_string(array_length);
        for (int k = 0; k < len_as_string.size(); k++) {
            // 8,000,000
            cout << len_as_string[k];
            if ((len_as_string.size() - k) % 3 == 1 && 
                 (len_as_string.size() - k) > 1) {
                cout << ".";
            }
        }
        cout << "\n";
    } else {
        cout << "running without verbose\n";
    }

    vector<Distr> dists = {normal, uniform, bimodal};

    string csv_body;
    if (existing_csv.size() > 0) {
        int num_lines = 0;
        string line;
        std::ifstream file(existing_csv);
        while (std::getline(file,line)) {
            csv_body += line + "\n";
            num_lines++;
        }
        cout << "Warning: running with existing csv file that has " 
             << (num_lines - 1) << " entries\n";
        output_fn = existing_csv;
    } else {
        string csv_body = string("dist,dist_param1,dist_param2,sync_time,") +
                      string("fine_lock_time,lock_free_time,num_threads,") + 
                      string("update_prob,removal_prob,array_len\n");
    }
    
    if (VERBOSE) cout << csv_body;
    string keys_csv = string("dist,dist_param1,dist_param2,array_length,key,op\n");
    string keys_fn = "keys_logging.csv";

    vector<Oper> ops = generate_ops(array_length, update_prob, removal_prob);
    if (VERBOSE) cout << "generated ops\n";
    for (unsigned int i = 0; i < dists.size(); i++) {
        Distr dist = dists[i];
        if (VERBOSE) {
            cout << "done with " << i << "/" << dists.size();
            cout << ", generating keys with distribution " << to_string_dist(dist)
                 << "\n";
        }
        vector<int> keys;
        string dist_info;
        double var = variance;
        if (dist == uniform) {
            int start = -1000;
            int end = 1000;
            keys = generate_keys(array_length,start,end,dist);
            dist_info = "uniform," + to_string(start) + "," + to_string(end);
            
        } else if (dist == normal) {
            double mean = 0.0;
            keys = generate_keys(array_length,mean,variance,dist);
            dist_info = "normal," + to_string(mean) + "," + to_string(var);
        } else if (dist == bimodal) {
            double mean = 0.0;
            var = variance / 4;
            keys = generate_keys(array_length,mean,var, dist);
            dist_info = "bimodal," + to_string(mean) + "," + to_string(var);
        }
        for (int j = 0; j < array_length; j++) {
            keys_csv += dist_info + "," + to_string(array_length) + "," +
                        to_string(keys[j]) + "," + to_string_op(ops[j]) + "\n";
        }
        if (VERBOSE) cout << "\tperforming run\n";

        for (unsigned int t = 0; t < thread_opts.size(); t++) {
            int num_threads = thread_opts[t];
            string s = benchmark_from_inputs(keys, ops, skip_prob, max_height, num_trials,
                              num_threads, array_length, update_prob, 
                              removal_prob, dist_info);
        if (VERBOSE) {
            cout << s;
        }
        csv_body += s;
        }
        

    }
    if (logging) {
        ofstream key_file(keys_fn);
        key_file << keys_csv;
        ofstream ofile("benchmark.csv");
        ofile << csv_body;
    }


}
// switch( c ) {
// case EASY:
//     DoStuff();
//     break;
// case MEDIUM:
//     ...
// }