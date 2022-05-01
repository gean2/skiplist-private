/**
 * Synchronized list AKA coarse-grained locking implementation.
 */

#include "skiplist.h"
#include <thread>
#include <mutex>
#include <bits/stdc++.h>
#include <iostream>

#ifndef SYNCLIST_H
#define SYNCLIST_H
template <typename T>
class Node {
    public:
    Node **_next;
    T *_value;
    int _key;
    int _top_level;
    Node(int key, T *value, int top_level)
            : _value(value), _key(key), _top_level(top_level) {
        _next = new Node<T> *[top_level];
    }
    ~Node() {
        delete[] _next;
    }
};

template <typename T>
class SyncList : public SkipList<T> {
    private:
    Node<T> *_leftmost;
    std::mutex _lock;

    public:
    SyncList(int max_level, double p) : SkipList<T>(max_level, p) {
        _leftmost = new Node<T>(INT_MIN, nullptr, this->_max_level);
        Node<T> *rightmost = new Node<T>(INT_MAX, nullptr, this->_max_level);
        for(int i = 0; i < this->_max_level; i++) {
            _leftmost->_next[i] = rightmost;
            rightmost->_next[i] = nullptr;
        }
    }

    ~SyncList() override {
        Node<T> *curr = _leftmost;
        Node<T> *next = curr->_next[0];
        while(next != nullptr) {
            delete curr;
            curr = next;
            next = next->_next[0];
        }
        delete curr;
    }

    T *update(int key, T *value) override {
        assert(key != INT_MIN && key != INT_MAX);
        _lock.lock();
        Node<T> *curr = _leftmost;
        Node<T> *updates[this->_max_level];
        for(int i = this->_max_level-1; i >= 0; i--) {
            while(curr->_next[i] != nullptr && curr->_next[i]->_key < key) {
                curr = curr->_next[i];
            }
            updates[i] = curr;
        }
        curr = curr->_next[0];
        if(curr != nullptr && key == curr->_key) {
            T *old_val = curr->_value;
            curr->_value = value;
            _lock.unlock();
            return old_val; // key is already in skip list
        }
        int level = SkipList<T>::rand_level();
        Node<T> *new_node = new Node<T>(key, value, level);
        for(int i = 0; i < level; i++) {
            new_node->_next[i] = updates[i]->_next[i];
            updates[i]->_next[i] = new_node;
        }
        _lock.unlock();
        return nullptr;
    }

    T *remove(int key) override  {
        _lock.lock();
        T *ret = nullptr;
        Node<T> *curr = _leftmost;
        Node<T> *updates[this->_max_level];
        for(int i = this->_max_level-1; i >= 0; i--) {
            while(curr->_next[i] != nullptr && curr->_next[i]->_key < key) {
                curr = curr->_next[i];
            }
            updates[i] = curr;
        }
        curr = curr->_next[0];
        if(curr->_key == key) {
            for(int i = 0; i < this->_max_level; i++) {
                if(updates[i]->_next[i] == curr) {
                    updates[i]->_next[i] = curr->_next[i];
                }
            }
            ret = curr->_value;
            delete curr;
        }
        _lock.unlock();
        return ret;
    }

    T *lookup(int key) override {
        _lock.lock();
        Node<T> *curr = _leftmost;
        for(int i = this->_max_level-1; i >= 0; i--) {
            while(curr->_next[i] != nullptr && curr->_next[i]->_key < key) {
                curr = curr->_next[i];
            }
        }
        curr = curr->_next[0];
        T *ret = (curr != nullptr && curr->_key == key) ? curr->_value : nullptr;
        _lock.unlock();
        return ret;
    }

    void print() override {
        std::cout << "synchronized skip list: ";
        for(int i = this->_max_level-1; i >= 0; i--) {
            Node<T> *curr = _leftmost;
            std::cout << "L" << i << ": ";
            while(curr->_next[i] != nullptr) {
                std::cout << curr->_key << ",";
                curr = curr->_next[i];
            }
            std::cout << curr->_key << "; ";
        }
        std::cout << "\n";
    }

    bool is_correct() { return true; }
};
#endif