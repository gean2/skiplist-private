/**
 * Synchronized list AKA coarse-grained locking implementation.
 */

#include "skiplist.h"
#include <thread>
#include <mutex>
#ifndef SYNCLIST_H
#define SYNCLIST_H
template <typename T>
class Node {
    public:
    Node **_next;
    T *_value;
    int _key;
    int _top_level;
    Node(int key, T *value, int top_level);
    ~Node();
};

template <typename T>
class SyncList : public SkipList<T> {
    private:
    Node<T> *_leftmost;
    std::mutex _lock;

    public:
    SyncList(int max_level, double p);
    ~SyncList();
    T *update(int key, T *value) override;
    T *remove(int key) override;
    T *lookup(int key) override;
    void print() override;
};
#endif