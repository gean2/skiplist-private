#include "include/lockfree.hpp"
#include "include/synclist.hpp"
#include "include/finelock.hpp"
#include "include/utils.h"
#include <cstdio>
#include <cstdlib>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>

static int _argc;
static const char **_argv;

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

int main(int argc, const char *argv[]) {
    using namespace std::chrono;
    typedef std::chrono::high_resolution_clock Clock;
    typedef std::chrono::duration<double> dsec;

    // Get command line arguments
    _argc = argc - 1;
    _argv = argv + 1;
    // skip list probability
    double skip_prob = get_option_float("-p", 0.5f); // probability of increasing a level
    int max_height = get_option_int("-h", 20); // maximum height of skip list

    bool no_sync = (bool) get_option_int("-ns", 0); // do not run benchmark for synchronized skip list
    int num_trials = get_option_int("-r", 5); // number of trials to average over
    int num_threads = get_option_int("-n", 8); // number of threads to run
    int array_length = get_option_int("-a", 10000000); // number of operations
    int dist = get_option_int("-dist", 0); // distribution; 0 is uniform distribution; 1 is normal distribution

    double update_prob = get_option_float("-i", 0.1f); // probability of update operation
    double removal_prob = get_option_float("-d", 0.1f); // probability of removal operation
    int variance = get_option_int("-v", 100000); // parameter used for input distribution

    // compute inputs
    std::vector<int> keys;
    std::vector<int> initial_keys;
    if(dist) {
        keys = generate_normal_keys(array_length, variance);
        initial_keys = generate_normal_keys(array_length/2, variance);
    } else {
        keys = generate_uniform_keys(array_length, variance);
        initial_keys = generate_uniform_keys(array_length/2, variance);
    } std::vector<int> ops = generate_ops(array_length, update_prob, removal_prob);
    std::vector<int> initial_ops(array_length/2, 0); // add a bunch of elements initially

    int max_deletions = std::count(ops.begin(), ops.end(), 1);

    // perform test
    double sync_time = 0;
    double lock_free_time = 0;
    double fine_lock_time = 0;

    if(!no_sync) {
        SyncList<int> *sl = new SyncList<int>(max_height, skip_prob);
        perform_test(sl, initial_keys, initial_ops, array_length/2, num_threads); // add initial elements
        perform_test(sl, keys, ops, array_length, num_threads);
        delete sl;
        for(int i = 0; i < num_trials; i++) {
            sl = new SyncList<int>(max_height, skip_prob);
            perform_test(sl, initial_keys, initial_ops, array_length/2, num_threads); // add initial elements
            auto compute_start = Clock::now();
            perform_test(sl, keys, ops, array_length, num_threads);
            sync_time += duration_cast<dsec>(Clock::now() - compute_start).count();
            delete sl;
        }
        sync_time /= num_trials;
    }

    FineLockList<int> *fl = new FineLockList<int>(max_height, skip_prob, max_deletions);
    perform_test(fl, initial_keys, initial_ops, array_length/2, num_threads); // add initial elements
    perform_test(fl, keys, ops, array_length, num_threads);
    delete fl;
    for(int i = 0; i < num_trials; i++) {
        fl = new FineLockList<int>(max_height, skip_prob, max_deletions);
        perform_test(fl, initial_keys, initial_ops, array_length/2, num_threads); // add initial elements
        auto compute_start = Clock::now();
        perform_test(fl, keys, ops, array_length, num_threads);
        fine_lock_time += duration_cast<dsec>(Clock::now() - compute_start).count();
        delete fl;
    }
    fine_lock_time /= num_trials;

    LockFreeList<int> *lf = new LockFreeList<int>(max_height, skip_prob, max_deletions);
    perform_test(lf, initial_keys, initial_ops, array_length/2, num_threads); // add initial elements
    perform_test(lf, keys, ops, array_length, num_threads);
    delete lf;
    for(int i = 0; i < num_trials; i++) {
        lf = new LockFreeList<int>(max_height, skip_prob, max_deletions);
        perform_test(lf, initial_keys, initial_ops, array_length/2, num_threads); // add initial elements
        auto compute_start = Clock::now();
        perform_test(lf, keys, ops, array_length, num_threads);
        lock_free_time += duration_cast<dsec>(Clock::now() - compute_start).count();
        delete lf;
    }
    lock_free_time /= num_trials;

    // print results
    std::cout << sync_time << "," << fine_lock_time << "," << lock_free_time << "," << num_threads << "," << update_prob << "," << removal_prob << "," << variance << "," << array_length << "\n";
}