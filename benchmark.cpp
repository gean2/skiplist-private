#include "include/lockfree.hpp"
#include "include/synclist.hpp"
#include "include/finelock.hpp"
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

vector<int> generate_normal_keys(int array_length, double var) {
    vector<int> v(array_length, 0);
    std::mt19937 gen{0};
    std::normal_distribution<> d{0, var};
    for(int i = 0; i < array_length; i++) {
        v[i] = std::round(d(gen));
    }
    return v;
}

vector<int> generate_ops(int array_length, double update_prob, double removal_prob) {
    std::mt19937 gen{0};
    std::uniform_real_distribution<> dist(0, 1);
    std::vector<int> res(array_length, 0);
    for (int i = 0; i < array_length; i++) {
        double g = dist(gen);
        if (g < update_prob) {
            res[i] = 0;
        }
        else if (g < update_prob + removal_prob) {
            res[i] = 1;
        }
        else {
            res[i] = 2;
        }
    }
    return res;
} 

double time(SkipList<int> *l, std::vector<int> keys, std::vector<int> ops, 
            int array_length, int num_threads) {
    using namespace std::chrono;
    double compute_time = 0;
    typedef std::chrono::high_resolution_clock Clock;
    typedef std::chrono::duration<double> dsec;
    auto compute_start = Clock::now();

    #pragma omp parallel for default(shared) schedule(dynamic) num_threads(num_threads)
    for(int i = 0; i < array_length; i++) {
        int *val;
        if(ops[i] == 0) {
            val = l->update(keys[i], &keys[i]);
        } else if(ops[i] == 1) {
            val = l->remove(keys[i]);
        } else {
            val = l->lookup(keys[i]);
        }
        assert(val == nullptr || *val == keys[i]);
    }
    compute_time += duration_cast<dsec>(Clock::now() - compute_start).count();
    return compute_time;
}

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

    // compute inputs
    std::vector<int> keys = generate_normal_keys(array_length, variance);
    std::vector<int> ops = generate_ops(array_length, update_prob, removal_prob);

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
    std::cout << sync_time << "," << fine_lock_time << "," << lock_free_time << "," << num_threads << "," << update_prob << "," << removal_prob << "," << variance << "," << array_length << "\n";
}