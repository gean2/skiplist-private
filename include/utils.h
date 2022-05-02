#include "skiplist.h"
#ifndef TEST_HELPER_H
#define TEST_HELPER_H
using std::vector;
enum Distr { normal, uniform, bimodal };
enum Oper {update_op, remove_op, lookup_op};

vector<int> generate_uniform_keys(int start, int end);
vector<int> generate_normal_keys(int array_length, double var);

vector<int> generate_bimodal_keys(float mean1, float var1,
                                  float mean2, float var2,
                                  float prob1);

vector<Oper> generate_ops(int array_length, double update_prob, double removal_prob);

double count_repeats(vector<int> vec);

void perform_test(SkipList<int> *l, std::vector<int> keys, std::vector<Oper> ops, 
                    int array_length, int num_threads);

#endif