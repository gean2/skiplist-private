#include "include/skiplist.h"
//#include "include/synclist.h" // TODO: correct declaration generates linker errors -> why?
//#include "include/lockfree.h"
#include "synclist.cpp" // TODO: WRONG RIGHT NOW
#include "lockfree.cpp"
#include <iostream>
#include <algorithm>
#include <random>
#include <assert.h>
#include <omp.h>

#define ARRAY_LENGTH 10000000
#define DELETION_RATIO 10

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

void generateInitial1(int *l1) {
    std::set<int> s1;
    while(s1.size() < ARRAY_LENGTH) {
        int val = (rand() % 1000000000) - 500000000;
        s1.insert(val);
    }
    int i = 0;
    for(std::set<int>::iterator itr = s1.begin(); itr != s1.end(); itr++) {
        l1[i] = *itr;
        i += 1;
    }
}

void generateInitial2(int *l1) {
    auto rng = std::default_random_engine {};
    std::vector<int> v;
    for(int i = 0; i < ARRAY_LENGTH; i++) {
        v.push_back(i);
    }
    std::shuffle(std::begin(v), std::end(v), rng);
    int i = 0;
    for(int val : v) {
        l1[i] = val;
        i++;
    }
}

void add_test1(SkipList<int> *l) {
    using namespace std::chrono;
    double init_time = 0;
    typedef std::chrono::high_resolution_clock Clock;
    typedef std::chrono::duration<double> dsec;

    int *l1 = new int[ARRAY_LENGTH];
    generateInitial2(l1);
    std::cout << "Done generating inputs\n";
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
    delete[] l1;
}

int main() {
    SyncList<int>l1(20, 0.5);
    //add_test0(&l1);
    //LockFreeList<int> l2(4, 0.5, 5);
    //add_test0(&l2);
    add_test1(&l1);
    LockFreeList<int> l3(20, 0.5, ARRAY_LENGTH/DELETION_RATIO);
    add_test1(&l3);
    return 0;
}