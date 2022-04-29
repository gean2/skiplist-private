#include "include/synclist.hpp"
#include "include/lockfree.hpp"
#include "include/finelock.hpp"
#include <iostream>
#include <algorithm>
#include <random>
#include <assert.h>
#include <omp.h>

#define ARRAY_LENGTH 100000000
#define DELETION_RATIO 2
using std::vector;


void add_test0(SkipList<int> *l) {
    int A[10] = {5,2,3,4,21,-2,33,12,6,7};
    int B[5] = {28, -4, 9, 10, 17};
    for(int i = 0; i < 10; i++) {
        //std::cout << "adding " << A[i] << "\n";
        assert(l->update(A[i], &A[i]) == nullptr);
        assert(l->update(A[i], &A[i]) == &A[i]);
    }
    for(int i = 0; i < 5; i++) {
        assert(l->update(B[i], &B[i]) == nullptr);
        assert(l->update(B[i], &B[i]) == &B[i]);
    }
    l->print();
    for(int i = 0; i < 10; i++) {
        assert(l->lookup(A[i]) == &A[i]);
    }
    for(int i = 0; i < 5; i++) {
        assert(l->lookup(B[i]) == &B[i]);
    }
    //std::cout << "starting removal (oh no)\n";
    for(int i = 0; i < 5; i++) {
        //std::cout << "removing " << B[i] << "\n";
        l->remove(B[i]);
    }
    for(int i = 0; i < 5; i++) {
        assert((l->lookup(B[i]) == nullptr));
    }
    for(int i = 0; i < 10; i++) {
        assert(l->lookup(A[i]) == &A[i]);
    }
    l->print();
    std::cout << "Passed add_test0\n";
}

vector<int> generate_initial2() {
    auto rng = std::default_random_engine {};
    vector<int> v(ARRAY_LENGTH, 0);
    for(int i = 0; i < ARRAY_LENGTH; i++) {
        v[i] = i;
    }
    std::shuffle(std::begin(v), std::end(v), rng);
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

// void count_repeats()

void add_test1(SkipList<int> *l) {
    using namespace std::chrono;
    double init_time = 0;
    typedef std::chrono::high_resolution_clock Clock;
    typedef std::chrono::duration<double> dsec;

    vector<int> l1 = generate_initial2();
    auto init_start = Clock::now();
    #pragma omp parallel for default(shared) schedule(dynamic) num_threads(8)
    for(int i = 0; i < ARRAY_LENGTH; i++) {
        assert(l->update(l1[i], &l1[i]) == nullptr);
    }
    #pragma omp parallel for default(shared) schedule(dynamic) num_threads(8)
    for(int i = 0; i < ARRAY_LENGTH; i+=DELETION_RATIO) {
        assert(l->remove(l1[i]) == &l1[i]);
    }
    #pragma omp parallel for default(shared) schedule(dynamic) num_threads(8)
    for(int i = 0; i < ARRAY_LENGTH; i++) {
        if(i % DELETION_RATIO) assert(l->lookup(l1[i]) == &l1[i]);
        else assert(l->lookup(l1[i]) == nullptr);
    }
    init_time += duration_cast<dsec>(Clock::now() - init_start).count();
    std::cout << "Passed add_test1 for " << init_time << " seconds.\n";
}

void add_test2(SkipList<int> *l) {
    using namespace std::chrono;
    double init_time = 0;
    typedef std::chrono::high_resolution_clock Clock;
    typedef std::chrono::duration<double> dsec;
    int dummy = 2;
    vector<int> A = generate_initial2();
    auto init_start = Clock::now();
    #pragma omp parallel for default(shared) schedule(dynamic)
    for(int i = 0; i < ARRAY_LENGTH * 2; i++) {
        if(i < ARRAY_LENGTH) {
            int *res = l->update(A[i], &A[i]);
            assert(res == &dummy || res == nullptr);
        } else {
            int idx = i % ARRAY_LENGTH;
            int *res;
            if(i % DELETION_RATIO) {
                res = l->update(A[idx], &dummy);
            }
            else {
                res = l->remove(A[idx]);
            }
            //if(res != nullptr && res != &A[idx]) std::cout << *res << " \n";
            assert(res == &A[idx] || res == nullptr);
        }
    }
    #pragma omp parallel for default(shared) schedule(dynamic)
    for(int i = 0; i < ARRAY_LENGTH; i++) {
        int *res = l->lookup(A[i]);
        if(i % DELETION_RATIO) {
            assert(res == &dummy || res == &A[i]);
        } else {
            assert(res == &A[i] || res == nullptr); 
        }
    }
    init_time += duration_cast<dsec>(Clock::now() - init_start).count();
    std::cout << "Passed add_test2 for " << init_time << " seconds.\n";
}

vector<int> generate_ops(double p_0, double p_1) {
    std::random_device rd;
    std::mt19937 gen{0};
    std::uniform_real_distribution<> dist(0, 1);
    std::vector<int> res(ARRAY_LENGTH, 0);
    for (int i = 0; i < ARRAY_LENGTH; i++) {
        double g = dist(gen);
        if (g < p_0) {
            res[i] = 0;
        }
        else if (g < p_0 + p_1) {
            res[i] = 1;
        }
        else {
            res[i] = 2;
        }
    }
    return res;
}

void normal_test0(SkipList<int> *l) {
    using namespace std::chrono;
    double init_time = 0;
    typedef std::chrono::high_resolution_clock Clock;
    typedef std::chrono::duration<double> dsec;
    vector<int> A = generate_normal_keys(0, 100000);
    vector<int> B = generate_ops(0.01, 0.01);
    // probability .1 of updating, .1 of deleting, and .8 of lookup
    std::cout << "Done generating inputs\n";
    auto init_start = Clock::now();
    #pragma omp parallel for default(shared) schedule(dynamic)
    for(int i = 0; i < ARRAY_LENGTH; i++) {
        int *val;
        if(B[i] == 0) {
            val = l->update(A[i], &A[i]);
        } else if(B[i] == 1) {
            val = l->remove(A[i]);
        } else {
            val = l->lookup(A[i]);
        }
        assert(val == nullptr || *val == A[i]);
    }
    init_time += duration_cast<dsec>(Clock::now() - init_start).count();
    std::cout << "Benchmarked normal for " << init_time << " seconds.\n";
}

int main() {
    SyncList<int>l1(4, 0.5);
    add_test0(&l1);
    //LockFreeList<int> l2(4, 0.5, 5);
    //add_test0(&l2);
    //add_test1(&l1);
    FineList<int> l3(20, 0.5, ARRAY_LENGTH/DELETION_RATIO);
    normal_test0(&l3);
    return 0;
}