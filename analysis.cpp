#include "include/skiplist.h"
#include "include/finelock.hpp"
#include "include/lockfree.hpp"
#include "include/synclist.hpp"
#include "include/utils.h"
#include <cstdio>
#include <cstdlib>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>

enum type{SYNC, FINELOCK, LOCKFREE};

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
    // Get command line arguments
    _argc = argc - 1;
    _argv = argv + 1;
    // skip list probability
    double skip_prob = get_option_float("-p", 0.5f); // probability of increasing a level
    int max_height = get_option_int("-h", 20); // maximum height of skip list

    int type = get_option_int("-s", 0);
    int dist = get_option_int("-dist", 0); // distribution; 0 is uniform distribution; 1 is normal distribution
    int num_threads = get_option_int("-n", 8);
    int array_length = get_option_int("-a", 10000000);

    double update_prob = get_option_float("-i", 0.1f);
    double removal_prob = get_option_float("-d", 0.1f);
    int variance = get_option_float("-v", 100000);

    // compute inputs
    std::vector<int> keys;
    std::vector<int> initial_keys;
    if(dist) {
        keys = generate_normal_keys(array_length, 0.0, variance);
        initial_keys = generate_normal_keys(array_length/2, 0.0, variance);
    } else {
        keys = generate_uniform_keys(array_length, -1 * variance, variance);
        initial_keys = generate_uniform_keys(array_length/2, -1 * variance,
                                             variance);
    } 
    std::vector<Oper> ops = generate_ops(array_length, update_prob, removal_prob);
    std::vector<Oper> initial_ops(array_length/2, update_op);
    // add a bunch of elements initially

    int max_deletions = std::count(ops.begin(), ops.end(), 1);
    SkipList<int> *l;
    if(type == FINELOCK) {
        std::cout << "Testing fine-grained locking skip list \n";
        l = new FineLockList<int>(max_height, skip_prob, max_deletions);
    } else if(type == LOCKFREE) {
        std::cout << "Testing lock-free skip list\n";
        l = new LockFreeList<int>(max_height, skip_prob, max_deletions);
    } else {
        std::cout << "Testing coarse-grained locking skip list\n";
        l = new SyncList<int>(max_height, skip_prob);
    }
    perform_test(l, initial_keys, initial_ops, array_length/2, num_threads); // add initial elements
    perform_test(l, keys, ops, array_length, num_threads);
    delete l;
}