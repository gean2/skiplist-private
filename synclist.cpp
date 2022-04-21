#include "include/synclist.h"
#include <bits/stdc++.h>
#include <iostream>
#include <assert.h>

template <typename T>
Node<T>::Node(int key, T *value, int top_level) 
        : _key(key), _value(value), _top_level(top_level) {
    _next = new Node *[top_level];
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
    }
}

template <typename T>
SyncList<T>::~SyncList() {
    Node<T> *curr = _leftmost;
    Node<T> *next = curr->_next[0];
    while(next != NULL) {
        delete curr;
        curr = next;
        next = next->_next[0];
    }
    delete curr;
}

template <typename T>
bool SyncList<T>::insert(int key, T *value) {
    assert(key != INT_MIN && key != INT_MAX);
    _lock.lock();
    Node<T> *curr = _leftmost;
    Node<T> *updates[this->_max_level];
    for(int i = this->_max_level-1; i >= 0; i--) {
        while(curr->_next[i] != NULL && curr->_next[i]->_key < key) {
            curr = curr->_next[i];
        }
        updates[i] = curr;
    }
    curr = curr->_next[0];
    if(curr != NULL && key == curr->_key) {
        _lock.unlock();
        return false; // key is already in skip list
    }
    int level = SkipList<T>::rand_level();
    Node<T> *new_node = new Node<T>(key, value, level);
    for(int i = 0; i < level; i++) {
        new_node->_next[i] = updates[i]->_next[i];
        updates[i]->_next[i] = new_node;
    }
    _lock.unlock();
    return true;
}

template <typename T>
T *SyncList<T>::remove(int key) {
    _lock.lock();
    T *ret = nullptr;
    Node<T> *curr = _leftmost;
    Node<T> *updates[this->_max_level];
    for(int i = this->_max_level-1; i >= 0; i--) {
        while(curr->_next[i] != NULL && curr->_next[i]->_key < key) {
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
        while(curr->_next[i] != NULL && curr->_next[i]->_key < key) {
            curr = curr->_next[i];
        }
    }
    T *ret = (curr != NULL && curr->_next[0]->_key == key) ? curr->_val : nullptr;
    _lock.unlock();
    return ret;
}

template<typename T>
void SyncList<T>::print() {
    std::cout << "synchronized skip list: ";
    for(int i = this->_max_level-1; i >= 0; i--) {
        Node<T> curr = _leftmost;
        std::cout << "L" << i << ": ";
        while(curr->right[i] != NULL) {
            std::cout << curr->key << ",";
            curr = curr->right[i];
        }
        std::cout << "; ";
    }
}

