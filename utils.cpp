#include "include/utils.h"
#include <iostream>
#include <algorithm>
#include <random>
#include <assert.h>
#include <limits.h>
#include <omp.h>
#include <math.h>
#include <cmath>
#include <random>

#define ARRAY_LENGTH 100000000
#define DELETION_RATIO 2
using std::vector;
#define VERBOSE false
#define NAN_1 nanf("1")

const int Num_Distrs = 3;
const int Num_Opers = 3;

vector<int> generate_initial2() {
    auto rng = std::default_random_engine {};
    vector<int> v(ARRAY_LENGTH, 0);
    for(int i = 0; i < ARRAY_LENGTH; i++) {
        v[i] = i;
    }
    std::shuffle(std::begin(v), std::end(v), rng);
    return v;
}

vector<int> generate_uniform_keys(int start, int end) {
    vector<int> v(ARRAY_LENGTH, 0);
    std::mt19937 gen{0};
    std::uniform_real_distribution<> d(start, end);
    for(int i = 0; i < ARRAY_LENGTH; i++) {
        v[i] = std::round(d(gen));
    }
    return v;
}

vector<int> generate_normal_keys(float mean, float var) {
    vector<int> v(ARRAY_LENGTH, 0);
    std::mt19937 gen{0};
    std::normal_distribution<> d{mean, var};
    for(int i = 0; i < ARRAY_LENGTH; i++) {
        v[i] = std::round(d(gen));
    }
    return v;
}

vector<int> generate_bimodal_keys(float mean1, float var1,
                                  float mean2, float var2,
                                  float prob1) {
    vector<int> v(ARRAY_LENGTH, 0);
    std::mt19937 gen1{0};
    std::normal_distribution<> d1{mean1, var1};
    std::mt19937 gen2{0};
    std::normal_distribution<> d2{mean2, var2};

    std::random_device rd;
    std::mt19937 gen3{0};
    std::uniform_real_distribution<> unif(0, 1);

    for (int i = 0; i < ARRAY_LENGTH; i++) {
        if (unif(gen3) < prob1) {
            v[i] = std::round(d1(gen1));
        } else {
            v[i] = std::round(d2(gen2));
        }
    }
    return v;
}

double count_repeats(vector<int> vec) {
    // int repeat, total = 0;
    if (VERBOSE) {
        std::cout << "frequencies (k: count):\n";
    }
    vector<int> v(vec); // copy of vec
    std::sort(v.begin(), v.end());
    int last = v[0];
    int last_i = 0;
    int unique = 1;
    for (int i = 1; i < ARRAY_LENGTH; i++) {
        if (v[i] != last) {
            if (VERBOSE) {
                std::cout << "\t" << last << ": " << (i - last_i) << "\n";
            }
            last = v[i];
            last_i = i;
            unique++;
        }
    }
    double unique_perc = (double)unique / (double)ARRAY_LENGTH;
    double res = (double)1.0 - unique_perc;
    return res;
}

vector<Oper> generate_ops(int array_length, double p_update, double p_remove) {
    std::random_device rd;
    std::mt19937 gen{0};
    std::uniform_real_distribution<> dist(0, 1);
    std::vector<Oper> res(array_length, lookup_op);
    for (int i = 0; i < array_length; i++) {
        double g = dist(gen);
        if (g < p_update) {
            res[i] = update_op;
        }
        else if (g < p_update + p_remove) {
            res[i] = remove_op;
        }
        else {
            res[i] = lookup_op;
        }
    }
    return res;
}

void perform_test(SkipList<int> *l, std::vector<int> keys, std::vector<Oper> ops, 
                    int array_length, int num_threads) {
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
}

vector<int> generate_keys_(float mean, float var, Distr dist,
                          float mean2=NAN_1, float var2=NAN_1,
                          float prob1=0.6) {
    // TODO seed?
    // vector<int> keys;
    if (dist == normal) {
        return generate_normal_keys(mean, var);
    } else if (dist == bimodal) {
        if (isnan(mean2) || isnan(var2)) {
            float mean1 = mean - (var * float(4));
            mean2 = mean + (var * float(4));
            return generate_bimodal_keys(mean1,var,mean2,var,prob1);
        } else {
            return generate_bimodal_keys(mean,var,mean2,var2,prob1);
        }
        
    } else {
        assert(dist == uniform);
        int start = mean;
        int end = var;
        if (VERBOSE && end - start < 5) {
            std::cout << "WARNING: Uniform called with close together ";
            std::cout << "U(" << start << "," << end << ")\n";
        }
        return generate_uniform_keys(start,end);
    }
}

vector<int> generate_keys(float mean, float var, Distr dist,
                          float mean2=NAN_1, float var2=NAN_1,
                          float prob1=.6) {
    vector<int> keys = generate_keys_(mean,var,dist,mean2,var2,prob1);
    for (unsigned int i = 0; i < keys.size(); i++) {
        if (keys[i] == INT_MAX) {
            keys[i]--;
        }
        if (keys[i] == INT_MIN) {
            keys[i]++;
        }
    }
    return keys;
}
