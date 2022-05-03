#include "skiplist.h"
#ifndef TEST_HELPER_H
#define TEST_HELPER_H
using std::vector;

vector<int> generate_uniform_keys(int array_length, int range);

vector<int> generate_normal_keys(int array_length, int variance);

vector<int> generate_ops(int array_length, double update_prob, double removal_prob);

void perform_test(SkipList<int> *l, std::vector<int> &keys, std::vector<int> &ops, 
                    int array_length, int num_threads);

#endif