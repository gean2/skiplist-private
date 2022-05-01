#include "include/lockfree.hpp"
#include "include/synclist.hpp"
#include "include/finelock.hpp"
#include <cstdio>
#include <cstdlib>
#include <omp.h>

int main(int argc, const char *argv[]) {
    // Get command line arguments
    _argc = argc - 1;
    _argv = argv + 1;
    int num_of_threads = get_option_int("-n", 1);
    double insertion_prob = get_option_float("-p1", 0.1f);
    double deletion_prob = get_option_float("-p2", 0.1f);
    int variance = get_option_float("-v", 1000);
    int array_length = get_option_int("-a", 10000000);

    // compute inputs

    // perform test

    // print results
}