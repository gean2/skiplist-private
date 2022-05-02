#include "include/lockfree.hpp"
#include "include/synclist.hpp"
#include "include/finelock.hpp"
#include "include/test_utils.hpp"
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

static int _argc;
static const char **_argv;

bool DEBUG = true;

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

double time(SkipList<int> *l, vector<int> keys, vector<Oper> ops, 
            int array_length, int num_threads) {
    using namespace std::chrono;
    double compute_time = 0;
    typedef std::chrono::high_resolution_clock Clock;
    typedef std::chrono::duration<double> dsec;
    auto compute_start = Clock::now();

    #pragma omp parallel for default(shared) schedule(dynamic) num_threads(num_threads)
    for(int i = 0; i < array_length; i++) {
        int *val;
        if(ops[i] == update_op) {
            val = l->update(keys[i], &keys[i]);
        } else if(ops[i] == remove_op) {
            val = l->remove(keys[i]);
        } else {
            val = l->lookup(keys[i]);
        }
        assert(val == nullptr || *val == keys[i]);
    }
    compute_time += duration_cast<dsec>(Clock::now() - compute_start).count();
    return compute_time;
}

string single_run(vector<int> &keys, vector<Oper> &ops,
                  double skip_prob, int max_height, int num_trials,
                  int num_threads, int array_length, double update_prob,
                  double removal_prob, int variance, Distr dist) {
        // compute inputs
    int max_deletions = std::count(ops.begin(), ops.end(), 2);

    // perform test
    double sync_time = 0;
    double lock_free_time = 0;
    double fine_lock_time = 0;

    SyncList<int> *sl = new SyncList<int>(max_height, skip_prob);
    time(sl, keys, ops, array_length, num_threads);
    delete sl;
    for(int i = 0; i < num_trials; i++) {
        sl = new SyncList<int>(max_height, skip_prob);
        sync_time += time(sl, keys, ops, array_length, num_threads);
        delete sl;
    }
    sync_time /= num_trials;

    FineLockList<int> *fl = new FineLockList<int>(max_height, skip_prob, max_deletions);
    time(fl, keys, ops, array_length, num_threads);
    delete fl;
    for(int i = 0; i < num_trials; i++) {
        fl = new FineLockList<int>(max_height, skip_prob, max_deletions);
        fine_lock_time += time(fl, keys, ops, array_length, num_threads);
        delete fl;
    }
    fine_lock_time /= num_trials;

    LockFreeList<int> *lf = new LockFreeList<int>(max_height, skip_prob, max_deletions);
    time(lf, keys, ops, array_length, num_threads);
    delete lf;
    for(int i = 0; i < num_trials; i++) {
        lf = new LockFreeList<int>(max_height, skip_prob, max_deletions);
        lock_free_time += time(lf, keys, ops, array_length, num_threads);
        delete lf;
    }
    lock_free_time /= num_trials;

    // print results
    std::cout << dist << ", " << sync_time << "," << fine_lock_time << "," << lock_free_time << "," << num_threads << "," << update_prob << "," << removal_prob << "," << variance << "," << array_length << "\n";
    return "";
}
/*
int main_diffopers(int argc, const char *argv[]) {

    // Get command line arguments
    _argc = argc - 1;
    _argv = argv + 1;
    // skip list probability
    double skip_prob = get_option_float("-p", 0.5f); // probability of increasing a level
    int max_height = get_option_int("-h", 20); // maximum height of skip list

    int num_trials = get_option_int("-r", 5);
    int num_threads = get_option_int("-n", 8);
    int array_length = get_option_int("-a", 10000000);

    // double update_prob = get_option_float("-i", 0.1f);
    // double removal_prob = get_option_float("-d", 0.1f);
    // int variance = get_option_float("-v", 100000);
    vector<int> keys = generate_bimodal_keys(array_length, variance);
    vector<Distr> dists = {normal, uniform, bimodal};
    // for (int i = 0; i < Num_Distrs; )
    
    vector<int> ops = generate_ops(array_length, update_prob, removal_prob);
    vector<vector<int>> keys(3);
    
    for (int i = 0; i < dists.size(); i++) {
        Distr dist_update = dists[i];
        keys[update_op] = 
        for (int j = 0; j < dists.size(); j++) {
            Distr dist_remove = dists[j];
            for (int k = 0; k < dists.size(); k++) {
                Distr dist_lookup = dists[j];

            }
        }
    }


}
*/

int main(int argc, const char *argv[]) {

    // Get command line arguments
    _argc = argc - 1;
    _argv = argv + 1;
    // skip list probability
    double skip_prob = get_option_float("-p", 0.5f); // probability of increasing a level
    int max_height = get_option_int("-h", 20); // maximum height of skip list

    int num_trials = get_option_int("-r", 5);
    int num_threads = get_option_int("-n", 8);
    int array_length = get_option_int("-a", 10000000);

    double update_prob = get_option_float("-i", 0.1f);
    double removal_prob = get_option_float("-d", 0.1f);
    int variance = get_option_float("-v", 100000);

    // vector<int> keys = generate_bimodal_keys(array_length, variance);
    vector<Distr> dists = {normal, uniform, bimodal};
    // for (int i = 0; i < Num_Distrs; )
    
    vector<Oper> ops = generate_ops(array_length, update_prob, removal_prob);
    using std::cout;
    if (DEBUG) cout << "generated ops\n";
    for (unsigned int i = 0; i < dists.size(); i++) {
        Distr dist = dists[i];
        if (DEBUG) cout << "generating keys with distribution " << dist << "\n";
        vector<int> keys;
        if (dist == uniform) {
            keys = generate_keys(-1000,1000, dist);
        } else if (dist == normal) {
            keys = generate_keys(0,10000, dist);
        } else if (dist == bimodal) {
            keys = generate_keys(0,variance / 4, dist);
        }
        if (DEBUG) cout << "performing run\n";
        string s = single_run(keys, ops, skip_prob, max_height, num_trials,
                              num_threads, array_length, update_prob, 
                              removal_prob, variance, dist);
    }


}
// switch( c ) {
// case EASY:
//     DoStuff();
//     break;
// case MEDIUM:
//     ...
// }