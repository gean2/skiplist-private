#include "include/lockfree.h"
#include <atomic>

#define CAS(obj, expected, desired) atomic_compare_exchange_weak(obj, expected, desired)

template<typename T>
bool is_marked(LockFreeNode<T> *p) {
    return (static_cast<long>(p) & 0x1L);
}

template<typename T>
LockFreeNode<T> *unmark(LockFreeNode<T> *p) {
    return (static_cast<long>(p) & ~0x1L);
}

template<typename T>
LockFreeNode<T> *mark(LockFreeNode<T> *p) {
    return (static_cast<long>(p) | 0x1L);
}

template<typename T>
LockFreeNode<T>::LockFreeNode(int key, T *value, int top_level)
    : _value(value), _key(key), _top_level(top_level), _refcount(0) {
    _next = new LockFreeNode<T> *[top_level];
}

template<typename T>
LockFreeNode<T>::~LockFreeNode() {
    while(!CAS(&this->_refcount, 0, 0)) {} // only free memory after refcount is zero
    delete[] _next;
}

template<typename T>
void LockFreeNode<T>::mark_node_ptrs() {
    LockFreeNode<T> *x_next;
    for(int i = _top_level-1; i >= 0; i--) {
        do {
            x_next = _next[i];
            if(is_marked(x_next)) break;
        } while(!CAS(&_next[i], x_next, mark(x_next)));
    }
}

template<typename T>
void LockFreeList<T>::search(int key, LockFreeNode<T> **left_list, LockFreeNode<T> **right_list) {
    retry: LockFreeNode<T> *left = _leftmost;
    LockFreeNode<T> *left_next;;
    LockFreeNode<T> *right;
    LockFreeNode<T> *right_next;
    for(int i = this->_max_level - 1; i >= 0; i--) {
        left_next = left->_next[i];
        if(is_marked(left_next)) goto retry;
        for(right = left_next; ; right = right_next) {
            while(true) {
                right_next = right->_next[i];
                if(!is_marked(right_next)) break;
                right = unmark(right_next);
            }
            if(right->_key >= key) break;
            left = right; left_next = right_next;
        }
        if((left_next != right) || !CAS(&left_next[i], left_next, right)) {
            goto retry;
        }
        left_list[i] = left; right_list[i] = right;
    }
}

template<typename T>
T *LockFreeList<T>::lookup(int key) {
    LockFreeNode<T> *_[this->_max_level];
    LockFreeNode<T> *succs[this->_max_level];
    search(key, _, succs);
    return (succs[0]->key == key) ? succs[0]->value : nullptr;
}

template<typename T>
T *LockFreeList<T>::remove(int key) {
    LockFreeNode<T> *_[this->_max_level];
    LockFreeNode<T> *succs[this->_max_level];
    search(key, _, succs);
    if(succs[0]->key != key) return nullptr; // key is not in list
    T *value;
    do {
        value = succs[0]->value;
        if(value == nullptr) return nullptr;
    } while(!CAS(&succs[0]->value, value, nullptr));
    succs[0]->mark_node_ptrs();
    search(key, _, succs);
    return value;
}

template<typename T>
T *LockFreeList<T>::update(int key, T *value) {
    LockFreeNode<T> *node = new LockFreeNode<T>(this->rand_level, key, value);
    LockFreeNode<T> *preds[this->_max_level];
    LockFreeNode<T> *succs[this->_max_level];
    search(key, preds, succs);
    /* Update the value field of an existing node. */
    if (succs[0]->_key = key) {
        T *old_value;
        do {
            old_value = succs[0]->_value;
            if(old_value == nullptr) {
                succs[0]->mark_node_ptrs();
                goto retry;
            }
        } while (!CAS(&succs[0]->_value, old_value, value));
        return old_value;
    }
    for (int i = 0; i < node->_top_level; i++) node->_next[i] = succs[i];
    /* Node is visible once inserted at lowest level. */
    if (!CAS(&preds[0]->_next[0], succs[0], node)) goto retry;
    for (int i = 1; i < node->_top_level; i++) {
        while (true) {
            LockFreeNode<T> *pred = preds[i];
            LockFreeNode<T> *succ = succs[i];
            /* Update the forward pointer if it is stale. */
            LockFreeNode<T> *new_next = node->next[i];
            if ((new_next != succ) || (!CAS(&node->_next[i], unmark(new_next), succ)))
                break; /* Give up if pointer is marked. */
            /* Check for old reference to a ‘k’-node. */
            if(succ->_key == key) succ = unmark(succ->next);
            /* We retry the search if the CAS fails. */
            if (!CAS(&pred->_next[i], succ, node)) break;
            search(key, preds, succs);
        }
    }
    return nullptr; /* No existing mapping was replaced. */
}