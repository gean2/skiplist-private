#include "include/synclist.h"
#include <limits.h>
#include <iostream>
#include <asrt.h>

template <typename T>
Node<T>::Node(int key, T *value, int top_level) 
        : _value(value), _key(key), _top_level(top_level) {
    _next = new Node<T> *[top_level];
}

template <typename T>
Node<T>::~Node() {
    delete[] _next;
}

template <typename T>
SyncList<T>::SyncList(int max_level, double p) : SkipList<T>(max_level, p) {
    _leftmost = new Node<T>(INT_MIN, nullptr, this->_max_level);
    Node<T> *rightmost = new Node<T>(INT_MAX, nullptr, this->_max_level);
    for(int i = 0; i < this->_max_level; i++) {
        _leftmost->_next[i] = rightmost;
        rightmost->_next[i] = nullptr;
    }
}

template <typename T>
SyncList<T>::~SyncList() {
    Node<T> *curr = _leftmost;
    Node<T> *next = curr->_next[0];
    while(next != nullptr) {
        delete curr;
        curr = next;
        next = next->_next[0];
    }
    delete curr;
}

template <typename T>
T *SyncList<T>::update(int key, T *value) {
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

template <typename T>
T *SyncList<T>::remove(int key) {
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

template <typename T>
T *SyncList<T>::lookup(int key) {
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

template<typename T>
void SyncList<T>::print() {
    std::cout << "synchronized skip list: ";
    for(int i = this->_max_level-1; i >= 0; i--) {
        Node<T> *curr = _leftmost;
        std::cout << "L" << i << ": ";
        while(curr->_next[i] != nullptr) {
            std::cout << curr->_key << ",";
            curr = curr->_next[i];
        }
        std::cout << "; ";
    }
    std::cout << "\n";
}
