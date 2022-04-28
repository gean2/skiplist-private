#include "include/skiplist.h"
//#include "include/synclist.h" // TODO: correct declaration generates linker errors -> why?
//#include "synclist.cpp" // TODO: WRONG RIGHT NOW
//#include "lockfree.cpp"
#include "finelock.cpp"
#include <iostream>
#include <assert.h>

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
    std::cout << "Passed add_test0 test\n";
}

int main() {
    //SyncList<int>l(4, 0.5);
    //add_test0(&l);
    //LockFreeList<int> l2(4, 0.5);
    //add_test0(&l2);
    FineList<int>l(4, 0.5);
    add_test0(&l);
    return 0;
}