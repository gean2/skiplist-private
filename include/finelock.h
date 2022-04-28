/**
 * fine-grained locking implementation, inspired by Herlily paper.
 */

#include "skiplist.h"
#include <thread>
#include <mutex>
// TODO include atomic?

#ifndef FINELOCK_H
#define FINELOCK_H
template <typename T>
class FineNode {
    public:
    FineNode **_next;
    T *_value;
    int _key;
    int _top_level;
    bool _fully_linked;
    bool _marked;
    int _refcount;
    std::mutex _lock;
    FineNode(int key, T *value, int top_level);
    ~FineNode();
    void mark_node_ptrs();
};

template <typename T>
class FineList : public SkipList<T> {
    private:
    FineNode<T> *_leftmost;
    std::mutex _lock;
    int search(int key, FineNode<T> **left_list, FineNode<T> **right_list);
    public:
    FineList(int max_level, double p);
    ~FineList();

    bool add(int key, T *value);
    // T *remove(int key);
    bool contains(int v);
    bool ok_to_delete(FineNode<T> *candidate, int lFound);

    T *update(int key, T *value) override;
    T *remove(int key) override;
    T *lookup(int key) override;
    void print() override;
};
#endif