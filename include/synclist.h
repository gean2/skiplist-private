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
    bool insert(int key, T *value);
    T *remove(int key);
    T *lookup(int key);
    void print();
};
#endif