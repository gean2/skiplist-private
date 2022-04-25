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
    LockFreeNode(int key, T *value, int top_level);
    ~LockFreeNode();
    void mark_node_ptrs();
};

template <typename T>
class LockFreeList : public SkipList<T> {
    private:
    LockFreeNode<T> *_leftmost; // header, etc.

    // below data structures are used to manage 
    LockFreeNode<T> **_deleted;
    std::atomic<int> _deletion_idx;
    int _max_deletion_idx;

    void search(int key, LockFreeNode<T> **left_list, LockFreeNode<T> **right_list);

    public:
    LockFreeList(int max_level, double p, int max_deletions=1000);

    /**
     * NOT THREAD-SAFE. To be called to destroy all memory associated with the
     * linked list (including deleted nodes that have not been freed yet)
     */
    ~LockFreeList();
    T *update(int key, T *value) override;
    T *remove(int key) override;
    T *lookup(int key) override;
    void print() override;
};
#endif