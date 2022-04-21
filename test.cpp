#include "include/skiplist.h"
//#include "include/synclist.h" // TODO: correct declaration generates linker errors -> why?
#include "synclist.cpp" // TODO: WRONG RIGHT NOW
#include <iostream>

#include <assert.h>

void add_test0(SkipList<int> *l) {
    int A[10] = {5,2,3,4,21,-2,33,12,6,7};
    int B[5] = {28, -4, 9, 10, 17};
    for(int i = 0; i < 10; i++) {
        assert(l->insert(A[i], &A[i]));
        assert(l->insert(A[i], &A[i]) == false);
    }
    for(int i = 0; i < 5; i++) {
        assert(l->insert(B[i], &B[i]));
        assert(l->insert(B[i], &B[i]) == false); // cannot add an element twice
    }
    l->print();
    for(int i = 0; i < 10; i++) {
        assert(l->lookup(A[i]) == &A[i]);
    }
    for(int i = 0; i < 5; i++) {
        assert(l->lookup(B[i]) == &B[i]);
    }
    for(int i = 0; i < 5; i++) {
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
    SyncList<int>l(4, 0.5);
    add_test0(&l);
    return 0;
}