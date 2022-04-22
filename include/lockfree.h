/**
 * Lock-free implementation of compare-and-swap
 */
#include "skiplist.h"
#include <atomic>
#ifndef LOCK_FREE_H
#define LOCK_FREE_H

template<typename T> 
class LockFreeNode{
    public:
    volatile std::atomic<LockFreeNode *>*_next;
    volatile std::atomic<T *>_value;
    int _key;
    int _top_level;
    volatile std::atomic<int> _refcount;
    LockFreeNode(int key, T *value, int top_level);
    ~LockFreeNode();
    void mark_node_ptrs();
};

template <typename T>
class LockFreeList : public SkipList<T> {
    private:
    LockFreeNode<T> *_leftmost;
    void search(int key, LockFreeNode<T> **left_list, LockFreeNode<T> **right_list);

    public:
    LockFreeList(int max_level, double p);
    ~LockFreeList();
    T *update(int key, T *value);
    T *remove(int key);
    T *lookup(int key);
};
#endif