#include "skiplist.h"
#ifndef TEST_HELPER_H
#define TEST_HELPER_H
using std::vector;
#define NAN_1 nanf("1")
enum Distr { normal, uniform, bimodal };
enum Oper {update_op, remove_op, lookup_op};

vector<int> generate_uniform_keys(int array_length, int start, int end);
vector<int> generate_normal_keys(int array_length, double mean, double var);

vector<int> generate_bimodal_keys(int array_length, double mean1, double var1,
                                  double mean2, double var2,
                                  double prob1);

vector<Oper> generate_ops(int array_length, double update_prob, double removal_prob);

double count_repeats(vector<int> vec);

void perform_test(SkipList<int> *l, std::vector<int> &keys, std::vector<Oper> &ops, 
                    int array_length, int num_threads);

vector<int> generate_keys(int array_length, double mean, double var, Distr dist,
                          double mean2=NAN_1, double var2=NAN_1, double prob1=.6);

std::string to_string_dist(Distr dist);
std::string to_string_op(Oper op);

#endif