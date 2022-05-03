#include "include/utils.h"
#include <assert.h>
#include <iostream>
#include <omp.h>
#include <random>

vector<int> generate_uniform_keys(int array_length, int range) {
    vector<int> v(array_length, 0);
    std::mt19937 gen{0};
    std::uniform_real_distribution<> d(-range/2, range/2);
    for(int i = 0; i < array_length; i++) {
        v[i] = std::round(d(gen));
    }
    return v;
}

vector<int> generate_normal_keys(int array_length, int variance) {
    vector<int> v(array_length, 0);
    std::mt19937 gen{0};
    std::normal_distribution<> d{0, static_cast<double>(variance)};
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

void perform_test(SkipList<int> *l, std::vector<int> &keys, std::vector<int> &ops, 
                    int array_length, int num_threads) {
    #pragma omp parallel for default(shared) schedule(dynamic) num_threads(num_threads)
    for(int i = 0; i < array_length; i++) {
        int *val = nullptr;
        if(ops[i] == 0) {
            val = l->update(keys[i], &keys[i]);
        } else if(ops[i] == 1) {
            val = l->remove(keys[i]);
        } else {
            val = l->lookup(keys[i]);
        }
        assert(val == nullptr || *val == keys[i]);
    }
}
